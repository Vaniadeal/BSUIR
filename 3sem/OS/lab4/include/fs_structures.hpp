#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

#pragma pack(push, 1)

namespace fs {

constexpr uint32_t FS_MAGIC = 0x4C4C4653; 
constexpr int32_t NO_BLOCK = -1;
constexpr size_t MAX_FILENAME_LEN = 31;
constexpr uint32_t DEFAULT_BLOCK_SIZE = 512;
constexpr uint32_t DEFAULT_TOTAL_BLOCKS = 2048; // 1 MB virtual disk
constexpr uint32_t DEFAULT_MAX_FILES = 64;

struct SuperBlock {
    uint32_t magic;              // FS Magic identifier
    uint32_t block_size;         // Size of a single block in bytes
    uint32_t total_blocks;       // Total blocks in virtual disk
    uint32_t free_blocks;        // Number of available data blocks
    uint32_t bitmap_start;       // First block of free block bitmap
    uint32_t bitmap_blocks;      // Number of blocks allocated for bitmap
    uint32_t dir_start;          // First block of single-level directory
    uint32_t dir_blocks;         // Number of blocks allocated for directory
    uint32_t data_start;         // First data block
    uint32_t max_files;          // Maximum number of files in directory
    uint8_t  padding[472];       // Padding to align to 512 bytes
};

struct DirectoryEntry {
    char     name[MAX_FILENAME_LEN + 1]; // File name (null-terminated)
    uint32_t size;                       // File size in bytes
    int32_t  first_block;                // Index of first block in chain (-1 if empty)
    int32_t  last_block;                 // Index of last block in chain (-1 if empty)
    uint64_t created_time;               // Timestamp of creation
    uint64_t modified_time;              // Timestamp of last modification
    uint8_t  is_used;                    // 1 = entry in use, 0 = free entry
    uint8_t  is_readonly;                // 1 = read-only, 0 = read-write
    uint8_t  padding[14];                // Pad to exactly 64 bytes
};

struct BlockHeader {
    int32_t  next_block;                 // Index of next block in chain (-1 = EOF)
    uint16_t data_size;                  // Number of valid data bytes in this block
    uint16_t flags;                      // Flags / future expansion
};

} // namespace fs

#pragma pack(pop)
