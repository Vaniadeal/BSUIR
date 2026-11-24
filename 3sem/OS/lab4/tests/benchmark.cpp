#include "block_device.hpp"
#include "file_system.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <filesystem>
#include <iomanip>

namespace fs_bench {

void run_random_access_benchmark(const std::string& out_csv) {
    std::cout << "[BENCHMARK 1] Running Random Access & Traversal Overhead benchmark...\n";
    std::ofstream csv(out_csv);
    csv << "blocks,file_size_bytes,seq_read_time_us,random_seek_time_us,traversed_hops\n";

    const uint32_t block_size = 512;
    const uint32_t payload_capacity = block_size - sizeof(fs::BlockHeader); // 504 bytes
    const uint32_t max_blocks = 250;
    const uint32_t total_disk_blocks = 2048;

    auto dev = std::make_shared<fs::MemoryBlockDevice>(block_size, total_disk_blocks);
    fs::FileSystem::format(*dev, 64);
    fs::FileSystem fs;
    fs.mount(dev);

    // Warm-up
    for (uint32_t num_blocks = 1; num_blocks <= max_blocks; num_blocks += 5) {
        std::string fname = "file_" + std::to_string(num_blocks);
        fs.create_file(fname);

        size_t file_bytes = num_blocks * payload_capacity;
        std::vector<uint8_t> data(file_bytes, 0x5A);
        fs.write_file(fname, data.data(), data.size(), false);

        // Measure sequential read time
        std::vector<uint8_t> read_buf;
        const int iters = 20;
        
        auto t_seq_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; ++i) {
            fs.read_file(fname, read_buf);
        }
        auto t_seq_end = std::chrono::high_resolution_clock::now();
        double seq_us = std::chrono::duration<double, std::micro>(t_seq_end - t_seq_start).count() / iters;

        // Measure random seek read time (reading the last block)
        std::vector<uint8_t> slice_buf;
        size_t hops = 0;
        size_t last_block_offset = (num_blocks - 1) * payload_capacity;
        
        auto t_seek_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; ++i) {
            fs.read_range(fname, last_block_offset, 100, slice_buf, hops);
        }
        auto t_seek_end = std::chrono::high_resolution_clock::now();
        double seek_us = std::chrono::duration<double, std::micro>(t_seek_end - t_seek_start).count() / iters;

        csv << num_blocks << "," << file_bytes << "," << std::fixed << std::setprecision(2)
            << seq_us << "," << seek_us << "," << hops << "\n";

        fs.delete_file(fname);
    }
    csv.close();
    std::cout << "Saved: " << out_csv << "\n";
}

void run_churn_fragmentation_benchmark(const std::string& out_csv) {
    std::cout << "[BENCHMARK 2] Running Churn & Fragmentation benchmark...\n";
    std::ofstream csv(out_csv);
    csv << "step,active_files,total_blocks,avg_hop_distance,fragmentation_ratio\n";

    const uint32_t block_size = 512;
    const uint32_t total_disk_blocks = 2048;
    const uint32_t payload_capacity = block_size - sizeof(fs::BlockHeader);

    auto dev = std::make_shared<fs::MemoryBlockDevice>(block_size, total_disk_blocks);
    fs::FileSystem::format(*dev, 256);
    fs::FileSystem fs;
    fs.mount(dev);

    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> size_dist(1, 15);

    std::vector<std::string> active_files;
    uint32_t file_counter = 0;

    for (int step = 1; step <= 300; ++step) {
        // Randomly create 2-3 files
        int to_create = 2;
        for (int c = 0; c < to_create; ++c) {
            if (active_files.size() >= 120 || fs.get_superblock().free_blocks < 20) {
                break;
            }
            std::string fname = "f_" + std::to_string(++file_counter) + ".dat";
            fs.create_file(fname);
            uint32_t blocks = size_dist(rng);
            std::vector<uint8_t> data(blocks * payload_capacity, 0xCC);
            try {
                fs.write_file(fname, data.data(), data.size(), false);
                active_files.push_back(fname);
            } catch (...) {
                fs.delete_file(fname);
            }
        }

        // Randomly delete 1 file if we have > 10 active files
        if (active_files.size() > 10 && (step % 2 == 0)) {
            std::uniform_int_distribution<size_t> del_dist(0, active_files.size() - 1);
            size_t idx = del_dist(rng);
            fs.delete_file(active_files[idx]);
            active_files.erase(active_files.begin() + idx);
        }

        if (step % 5 == 0) {
            auto frag = fs.get_fragmentation_stats();
            csv << step << "," << frag.total_files << "," << frag.total_allocated_blocks << ","
                << std::fixed << std::setprecision(2) << frag.avg_hop_distance << ","
                << frag.fragmentation_ratio << "\n";
        }
    }
    csv.close();
    std::cout << "Saved: " << out_csv << "\n";
}

void run_disk_fill_benchmark(const std::string& out_csv) {
    std::cout << "[BENCHMARK 3] Running Disk Fill Ratio vs Allocation Time benchmark...\n";
    std::ofstream csv(out_csv);
    csv << "fill_percentage,allocated_blocks,alloc_latency_us\n";

    const uint32_t block_size = 512;
    const uint32_t total_disk_blocks = 2048;
    const uint32_t payload_capacity = block_size - sizeof(fs::BlockHeader);

    auto dev = std::make_shared<fs::MemoryBlockDevice>(block_size, total_disk_blocks);
    fs::FileSystem::format(*dev, 512);
    fs::FileSystem fs;
    fs.mount(dev);

    uint32_t initial_free = fs.get_superblock().free_blocks;
    uint32_t file_id = 0;

    for (int target_pct = 5; target_pct <= 95; target_pct += 5) {
        uint32_t desired_used = (initial_free * target_pct) / 100;
        uint32_t current_used = initial_free - fs.get_superblock().free_blocks;

        while (current_used < desired_used && fs.get_superblock().free_blocks > 10) {
            std::string fill_file = "fill_" + std::to_string(++file_id) + ".bin";
            uint32_t blks = std::min<uint32_t>(5, desired_used - current_used);
            if (blks == 0) blks = 1;
            fs.create_file(fill_file);
            std::vector<uint8_t> data(blks * payload_capacity, 0x11);
            fs.write_file(fill_file, data.data(), data.size(), false);
            current_used = initial_free - fs.get_superblock().free_blocks;
        }

        // Now measure allocation latency for a 5-block test file
        std::string test_fname = "probe_test.bin";
        fs.create_file(test_fname);
        std::vector<uint8_t> probe_data(5 * payload_capacity, 0xEE);

        auto t0 = std::chrono::high_resolution_clock::now();
        fs.write_file(test_fname, probe_data.data(), probe_data.size(), false);
        auto t1 = std::chrono::high_resolution_clock::now();
        double alloc_us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        fs.delete_file(test_fname);

        csv << target_pct << "," << current_used << ","
            << std::fixed << std::setprecision(2) << alloc_us << "\n";
    }
    csv.close();
    std::cout << "Saved: " << out_csv << "\n";
}

void run_efficiency_benchmark(const std::string& out_csv) {
    std::cout << "[BENCHMARK 4] Running Storage Efficiency vs Block Size benchmark...\n";
    std::ofstream csv(out_csv);
    csv << "block_size,header_size,payload_size,efficiency_pct\n";

    std::vector<uint32_t> block_sizes = {128, 256, 512, 1024, 2048, 4096};
    const uint32_t header_size = sizeof(fs::BlockHeader); // 8 bytes

    for (uint32_t bs : block_sizes) {
        uint32_t payload = bs - header_size;
        double eff = (static_cast<double>(payload) / bs) * 100.0;
        csv << bs << "," << header_size << "," << payload << ","
            << std::fixed << std::setprecision(2) << eff << "\n";
    }
    csv.close();
    std::cout << "Saved: " << out_csv << "\n";
}

} // namespace fs_bench

int main() {
    std::cout << "========================================================\n";
    std::cout << "  RUNNING LOAD TESTS & PERFORMANCE BENCHMARKS           \n";
    std::cout << "========================================================\n";

    std::filesystem::create_directories("benchmarks/data");
    std::filesystem::create_directories("benchmarks/charts");

    fs_bench::run_random_access_benchmark("benchmarks/data/random_access.csv");
    fs_bench::run_churn_fragmentation_benchmark("benchmarks/data/churn_fragmentation.csv");
    fs_bench::run_disk_fill_benchmark("benchmarks/data/disk_fill.csv");
    fs_bench::run_efficiency_benchmark("benchmarks/data/block_efficiency.csv");

    std::cout << "========================================================\n";
    std::cout << "  ALL BENCHMARKS COMPLETED SUCCESSFULLY!                \n";
    std::cout << "========================================================\n";
    return 0;
}
