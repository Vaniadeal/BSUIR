#include "block_device.hpp"
#include "file_system.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <sstream>

void test_create_and_delete() {
    std::cout << "[TEST] Running test_create_and_delete... ";
    auto dev = std::make_shared<fs::MemoryBlockDevice>(512, 1024);
    fs::FileSystem::format(*dev);
    fs::FileSystem fs;
    fs.mount(dev);

    assert(fs.list_files().empty());
    fs.create_file("test.txt");
    assert(fs.file_exists("test.txt"));
    assert(!fs.file_exists("nonexistent.txt"));

    auto info = fs.get_file_info("test.txt");
    assert(info.name == "test.txt");
    assert(info.size == 0);
    assert(info.first_block == fs::NO_BLOCK);

    fs.delete_file("test.txt");
    assert(!fs.file_exists("test.txt"));
    assert(fs.list_files().empty());
    std::cout << "PASSED\n";
}

void test_write_and_read_small() {
    std::cout << "[TEST] Running test_write_and_read_small... ";
    auto dev = std::make_shared<fs::MemoryBlockDevice>(512, 1024);
    fs::FileSystem::format(*dev);
    fs::FileSystem fs;
    fs.mount(dev);

    fs.create_file("small.txt");
    std::string text = "Operating Systems Lab 4 - Linked List of Blocks";
    fs.write_file("small.txt", text.data(), text.size(), false);

    auto info = fs.get_file_info("small.txt");
    assert(info.size == text.size());
    assert(info.blocks_count == 1);
    assert(info.first_block != fs::NO_BLOCK);
    assert(info.first_block == info.last_block);

    std::vector<uint8_t> read_buf;
    fs.read_file("small.txt", read_buf);
    std::string read_str(read_buf.begin(), read_buf.end());
    assert(read_str == text);
    std::cout << "PASSED\n";
}

void test_write_and_read_multi_block() {
    std::cout << "[TEST] Running test_write_and_read_multi_block... ";
    auto dev = std::make_shared<fs::MemoryBlockDevice>(512, 1024);
    fs::FileSystem::format(*dev);
    fs::FileSystem fs;
    fs.mount(dev);

    fs.create_file("multi.bin");
    // Block payload is 512 - 8 = 504 bytes. Let's write 2500 bytes (5 blocks: 4*504 + 484 = 2500)
    std::vector<uint8_t> pattern(2500);
    for (size_t i = 0; i < pattern.size(); ++i) {
        pattern[i] = static_cast<uint8_t>(i % 256);
    }
    fs.write_file("multi.bin", pattern.data(), pattern.size(), false);

    auto info = fs.get_file_info("multi.bin");
    assert(info.size == 2500);
    assert(info.blocks_count == 5);
    assert(info.block_chain.size() == 5);

    // Verify chain is valid
    for (size_t i = 0; i < info.block_chain.size() - 1; ++i) {
        assert(info.block_chain[i] != info.block_chain[i + 1]);
    }

    std::vector<uint8_t> read_buf;
    fs.read_file("multi.bin", read_buf);
    assert(read_buf == pattern);
    std::cout << "PASSED\n";
}

void test_append() {
    std::cout << "[TEST] Running test_append... ";
    auto dev = std::make_shared<fs::MemoryBlockDevice>(512, 1024);
    fs::FileSystem::format(*dev);
    fs::FileSystem fs;
    fs.mount(dev);

    fs.create_file("append.txt");
    std::string part1 = "Part 1: Header content. ";
    std::string part2 = "Part 2: Additional body content across blocks.";

    fs.write_file("append.txt", part1.data(), part1.size(), false);
    fs.write_file("append.txt", part2.data(), part2.size(), true); // append = true

    std::vector<uint8_t> buf;
    fs.read_file("append.txt", buf);
    std::string full_text(buf.begin(), buf.end());
    assert(full_text == (part1 + part2));
    std::cout << "PASSED\n";
}

void test_copy_and_move() {
    std::cout << "[TEST] Running test_copy_and_move... ";
    auto dev = std::make_shared<fs::MemoryBlockDevice>(512, 1024);
    fs::FileSystem::format(*dev);
    fs::FileSystem fs;
    fs.mount(dev);

    fs.create_file("original.txt");
    std::string data = "Copy move test data content.";
    fs.write_file("original.txt", data.data(), data.size(), false);

    // Copy
    fs.copy_file("original.txt", "copied.txt");
    assert(fs.file_exists("original.txt"));
    assert(fs.file_exists("copied.txt"));

    auto orig_info = fs.get_file_info("original.txt");
    auto copy_info = fs.get_file_info("copied.txt");
    assert(orig_info.size == copy_info.size);
    // Block chains must be distinct!
    assert(orig_info.first_block != copy_info.first_block);

    std::vector<uint8_t> copy_data;
    fs.read_file("copied.txt", copy_data);
    assert(std::string(copy_data.begin(), copy_data.end()) == data);

    // Move / Rename
    fs.move_file("original.txt", "renamed.txt");
    assert(!fs.file_exists("original.txt"));
    assert(fs.file_exists("renamed.txt"));

    auto ren_info = fs.get_file_info("renamed.txt");
    assert(ren_info.first_block == orig_info.first_block); // blocks preserved!
    std::cout << "PASSED\n";
}

void test_free_blocks_recovery() {
    std::cout << "[TEST] Running test_free_blocks_recovery... ";
    auto dev = std::make_shared<fs::MemoryBlockDevice>(512, 1024);
    fs::FileSystem::format(*dev);
    fs::FileSystem fs;
    fs.mount(dev);

    uint32_t initial_free = fs.get_superblock().free_blocks;

    fs.create_file("temp.bin");
    std::vector<uint8_t> large(504 * 10, 0xFF); // exactly 10 blocks
    fs.write_file("temp.bin", large.data(), large.size(), false);

    assert(fs.get_superblock().free_blocks == initial_free - 10);

    fs.delete_file("temp.bin");
    assert(fs.get_superblock().free_blocks == initial_free);
    std::cout << "PASSED\n";
}

void test_dump() {
    std::cout << "[TEST] Running test_dump... ";
    auto dev = std::make_shared<fs::MemoryBlockDevice>(512, 128);
    fs::FileSystem::format(*dev);
    fs::FileSystem fs;
    fs.mount(dev);

    fs.create_file("alpha.txt");
    fs.write_file("alpha.txt", "Alpha data", 10, false);
    fs.create_file("beta.txt");
    fs.write_file("beta.txt", "Beta data stream", 16, false);

    std::ostringstream oss;
    fs.dump(oss, true);
    std::string dump_str = oss.str();

    assert(dump_str.find("SUPERBLOCK INFO") != std::string::npos);
    assert(dump_str.find("alpha.txt") != std::string::npos);
    assert(dump_str.find("beta.txt") != std::string::npos);
    assert(dump_str.find("BLOCK ALLOCATION BITMAP") != std::string::npos);
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "   RUNNING UNIT TESTS (VARIANT 2)       \n";
    std::cout << "========================================\n";

    try {
        test_create_and_delete();
        test_write_and_read_small();
        test_write_and_read_multi_block();
        test_append();
        test_copy_and_move();
        test_free_blocks_recovery();
        test_dump();
        std::cout << "========================================\n";
        std::cout << "   ALL UNIT TESTS PASSED SUCCESSFULLY!  \n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[TEST FAILED]: " << e.what() << "\n";
        return 1;
    }
}
