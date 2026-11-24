#include "block_device.hpp"
#include <cstring>
#include <iostream>
#include <filesystem>

namespace fs {

// --- MemoryBlockDevice ---

MemoryBlockDevice::MemoryBlockDevice(uint32_t block_size, uint32_t total_blocks)
    : block_size_(block_size), total_blocks_(total_blocks), storage_(static_cast<size_t>(block_size) * total_blocks, 0) {
    if (block_size == 0 || total_blocks == 0) {
        throw std::invalid_argument("Block size and total blocks must be non-zero");
    }
}

void MemoryBlockDevice::read_block(uint32_t block_id, void* buffer) {
    if (block_id >= total_blocks_) {
        throw std::out_of_range("Block id out of range: " + std::to_string(block_id));
    }
    if (!buffer) {
        throw std::invalid_argument("Buffer cannot be null");
    }
    std::memcpy(buffer, storage_.data() + static_cast<size_t>(block_id) * block_size_, block_size_);
    ++read_ops_;
}

void MemoryBlockDevice::write_block(uint32_t block_id, const void* buffer) {
    if (block_id >= total_blocks_) {
        throw std::out_of_range("Block id out of range: " + std::to_string(block_id));
    }
    if (!buffer) {
        throw std::invalid_argument("Buffer cannot be null");
    }
    std::memcpy(storage_.data() + static_cast<size_t>(block_id) * block_size_, buffer, block_size_);
    ++write_ops_;
}

// --- FileBlockDevice ---

FileBlockDevice::FileBlockDevice(const std::string& filepath, uint32_t block_size, uint32_t total_blocks, bool create_if_missing)
    : filepath_(filepath), block_size_(block_size), total_blocks_(total_blocks) {
    if (block_size == 0 || total_blocks == 0) {
        throw std::invalid_argument("Block size and total blocks must be non-zero");
    }

    size_t expected_size = static_cast<size_t>(block_size) * total_blocks;
    bool exists = std::filesystem::exists(filepath);

    if (!exists && !create_if_missing) {
        throw std::runtime_error("File does not exist: " + filepath);
    }

    if (!exists || std::filesystem::file_size(filepath) != expected_size) {
        // Create / resize file
        std::ofstream out(filepath, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Failed to create file: " + filepath);
        }
        std::vector<uint8_t> zero_buf(block_size, 0);
        for (uint32_t i = 0; i < total_blocks; ++i) {
            out.write(reinterpret_cast<const char*>(zero_buf.data()), block_size);
        }
        out.close();
    }

    file_.open(filepath, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open file for read/write: " + filepath);
    }
}

FileBlockDevice::~FileBlockDevice() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void FileBlockDevice::read_block(uint32_t block_id, void* buffer) {
    if (block_id >= total_blocks_) {
        throw std::out_of_range("Block id out of range: " + std::to_string(block_id));
    }
    if (!buffer) {
        throw std::invalid_argument("Buffer cannot be null");
    }

    file_.seekg(static_cast<std::streamoff>(block_id) * block_size_, std::ios::beg);
    if (!file_) {
        throw std::runtime_error("Failed to seek to block " + std::to_string(block_id));
    }
    file_.read(reinterpret_cast<char*>(buffer), block_size_);
    if (!file_) {
        throw std::runtime_error("Failed to read block " + std::to_string(block_id));
    }
    ++read_ops_;
}

void FileBlockDevice::write_block(uint32_t block_id, const void* buffer) {
    if (block_id >= total_blocks_) {
        throw std::out_of_range("Block id out of range: " + std::to_string(block_id));
    }
    if (!buffer) {
        throw std::invalid_argument("Buffer cannot be null");
    }

    file_.seekp(static_cast<std::streamoff>(block_id) * block_size_, std::ios::beg);
    if (!file_) {
        throw std::runtime_error("Failed to seek to block " + std::to_string(block_id));
    }
    file_.write(reinterpret_cast<const char*>(buffer), block_size_);
    if (!file_) {
        throw std::runtime_error("Failed to write block " + std::to_string(block_id));
    }
    ++write_ops_;
}

void FileBlockDevice::sync() {
    if (file_.is_open()) {
        file_.flush();
    }
}

} // namespace fs
