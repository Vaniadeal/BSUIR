#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <stdexcept>

namespace fs {

class BlockDevice {
public:
    virtual ~BlockDevice() = default;

    virtual void read_block(uint32_t block_id, void* buffer) = 0;
    virtual void write_block(uint32_t block_id, const void* buffer) = 0;
    virtual uint32_t get_block_size() const = 0;
    virtual uint32_t get_total_blocks() const = 0;
    virtual void sync() = 0;

    // Statistics
    virtual uint64_t get_read_ops() const = 0;
    virtual uint64_t get_write_ops() const = 0;
    virtual void reset_stats() = 0;
};

// In-Memory Block Device (fast, ideal for benchmarks and transient testing)
class MemoryBlockDevice : public BlockDevice {
public:
    MemoryBlockDevice(uint32_t block_size, uint32_t total_blocks);
    ~MemoryBlockDevice() override = default;

    void read_block(uint32_t block_id, void* buffer) override;
    void write_block(uint32_t block_id, const void* buffer) override;
    uint32_t get_block_size() const override { return block_size_; }
    uint32_t get_total_blocks() const override { return total_blocks_; }
    void sync() override {}

    uint64_t get_read_ops() const override { return read_ops_; }
    uint64_t get_write_ops() const override { return write_ops_; }
    void reset_stats() override { read_ops_ = 0; write_ops_ = 0; }

    const std::vector<uint8_t>& raw_storage() const { return storage_; }

private:
    uint32_t block_size_;
    uint32_t total_blocks_;
    std::vector<uint8_t> storage_;
    mutable uint64_t read_ops_{0};
    mutable uint64_t write_ops_{0};
};

// File-backed Block Device (persistent disk image)
class FileBlockDevice : public BlockDevice {
public:
    FileBlockDevice(const std::string& filepath, uint32_t block_size, uint32_t total_blocks, bool create_if_missing = true);
    ~FileBlockDevice() override;

    void read_block(uint32_t block_id, void* buffer) override;
    void write_block(uint32_t block_id, const void* buffer) override;
    uint32_t get_block_size() const override { return block_size_; }
    uint32_t get_total_blocks() const override { return total_blocks_; }
    void sync() override;

    uint64_t get_read_ops() const override { return read_ops_; }
    uint64_t get_write_ops() const override { return write_ops_; }
    void reset_stats() override { read_ops_ = 0; write_ops_ = 0; }

private:
    std::string filepath_;
    uint32_t block_size_;
    uint32_t total_blocks_;
    std::fstream file_;
    mutable uint64_t read_ops_{0};
    mutable uint64_t write_ops_{0};
};

} // namespace fs
