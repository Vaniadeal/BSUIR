#pragma once

#include "block_device.hpp"
#include "fs_structures.hpp"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <chrono>

namespace fs {

struct FileInfo {
    std::string name;
    uint32_t size;
    uint32_t blocks_count;
    int32_t first_block;
    int32_t last_block;
    uint64_t created_time;
    uint64_t modified_time;
    bool is_readonly;
    std::vector<int32_t> block_chain;
};

struct FragmentationStats {
    uint32_t total_files;
    uint32_t total_allocated_blocks;
    uint32_t total_hops;
    double avg_hop_distance;
    double fragmentation_ratio; // % of hops where distance != 1
};

class FileSystem {
public:
    FileSystem();
    ~FileSystem() = default;

    // Formatting & Mounting
    static void format(BlockDevice& device, uint32_t max_files = DEFAULT_MAX_FILES);
    void mount(std::shared_ptr<BlockDevice> device);
    void unmount();
    bool is_mounted() const { return device_ != nullptr && mounted_; }

    // Core File Operations
    void create_file(const std::string& filename);
    void delete_file(const std::string& filename);
    void copy_file(const std::string& src_name, const std::string& dst_name);
    void move_file(const std::string& src_name, const std::string& dst_name); // Rename in flat FS
    
    // Read & Write
    size_t write_file(const std::string& filename, const void* data, size_t size, bool append = false);
    size_t read_file(const std::string& filename, std::vector<uint8_t>& buffer);
    size_t read_range(const std::string& filename, size_t offset, size_t length, std::vector<uint8_t>& buffer, size_t& blocks_traversed);

    // Metadata & Directory Inspection
    std::vector<FileInfo> list_files();
    FileInfo get_file_info(const std::string& filename);
    bool file_exists(const std::string& filename);

    // Diagnostics & Dump
    void dump(std::ostream& os, bool include_block_contents = true, const std::string& file_filter = "");
    FragmentationStats get_fragmentation_stats();
    const SuperBlock& get_superblock() const { return sb_; }
    std::shared_ptr<BlockDevice> get_device() const { return device_; }

private:
    std::shared_ptr<BlockDevice> device_;
    bool mounted_{false};
    SuperBlock sb_{};
    uint32_t payload_capacity_{0};

    // Internal helpers
    void read_superblock();
    void write_superblock();
    
    bool get_bitmap_bit(uint32_t block_id);
    void set_bitmap_bit(uint32_t block_id, bool used);
    int32_t allocate_free_block();
    void free_block(uint32_t block_id);

    int find_dir_entry_index(const std::string& filename, DirectoryEntry& out_entry);
    int find_free_dir_entry_index();
    void read_dir_entry(int entry_index, DirectoryEntry& entry);
    void write_dir_entry(int entry_index, const DirectoryEntry& entry);

    std::vector<int32_t> get_block_chain(int32_t first_block);
    void free_block_chain(int32_t first_block);
};

} // namespace fs
