#include "file_system.hpp"
#include <cstring>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace fs {

static uint64_t current_time_seconds() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

FileSystem::FileSystem() = default;

void FileSystem::format(BlockDevice& device, uint32_t max_files) {
    uint32_t block_size = device.get_block_size();
    uint32_t total_blocks = device.get_total_blocks();

    if (block_size <= sizeof(BlockHeader)) {
        throw std::invalid_argument("Block size is too small for BlockHeader");
    }

    uint32_t entries_per_block = block_size / sizeof(DirectoryEntry);
    if (entries_per_block == 0) {
        throw std::invalid_argument("Block size is too small for DirectoryEntry");
    }

    // Bitmap blocks: 1 bit per disk block
    uint32_t bitmap_blocks = (total_blocks + (block_size * 8) - 1) / (block_size * 8);

    // Directory blocks
    uint32_t dir_blocks = (max_files + entries_per_block - 1) / entries_per_block;
    uint32_t actual_max_files = dir_blocks * entries_per_block;

    // Data blocks start after superblock (1) + bitmap_blocks + dir_blocks
    uint32_t data_start = 1 + bitmap_blocks + dir_blocks;
    if (data_start >= total_blocks) {
        throw std::runtime_error("Device too small for metadata layout");
    }

    uint32_t free_blocks = total_blocks - data_start;

    // 1. Write Superblock
    SuperBlock sb{};
    sb.magic = FS_MAGIC;
    sb.block_size = block_size;
    sb.total_blocks = total_blocks;
    sb.free_blocks = free_blocks;
    sb.bitmap_start = 1;
    sb.bitmap_blocks = bitmap_blocks;
    sb.dir_start = 1 + bitmap_blocks;
    sb.dir_blocks = dir_blocks;
    sb.data_start = data_start;
    sb.max_files = actual_max_files;

    std::vector<uint8_t> block_buf(block_size, 0);
    std::memcpy(block_buf.data(), &sb, sizeof(SuperBlock));
    device.write_block(0, block_buf.data());

    // 2. Initialize Bitmap:
    // Blocks [0 .. data_start - 1] are reserved for system metadata (set bit 1)
    // Blocks [data_start .. total_blocks - 1] are free (set bit 0)
    for (uint32_t b = 0; b < bitmap_blocks; ++b) {
        std::fill(block_buf.begin(), block_buf.end(), 0);
        uint32_t start_bit = b * block_size * 8;
        for (uint32_t i = 0; i < block_size * 8; ++i) {
            uint32_t blk = start_bit + i;
            if (blk < data_start) {
                // System block
                block_buf[i / 8] |= (1 << (i % 8));
            } else if (blk >= total_blocks) {
                // Out of range, mark as used to prevent allocation
                block_buf[i / 8] |= (1 << (i % 8));
            }
        }
        device.write_block(sb.bitmap_start + b, block_buf.data());
    }

    // 3. Clear Directory blocks
    std::fill(block_buf.begin(), block_buf.end(), 0);
    for (uint32_t d = 0; d < dir_blocks; ++d) {
        device.write_block(sb.dir_start + d, block_buf.data());
    }

    device.sync();
}

void FileSystem::mount(std::shared_ptr<BlockDevice> device) {
    if (!device) {
        throw std::invalid_argument("Device cannot be null");
    }
    device_ = device;
    read_superblock();

    if (sb_.magic != FS_MAGIC) {
        device_ = nullptr;
        throw std::runtime_error("Invalid filesystem magic signature");
    }
    if (sb_.block_size != device_->get_block_size()) {
        device_ = nullptr;
        throw std::runtime_error("Device block size mismatch");
    }
    if (sb_.total_blocks != device_->get_total_blocks()) {
        device_ = nullptr;
        throw std::runtime_error("Device total blocks mismatch");
    }

    payload_capacity_ = sb_.block_size - sizeof(BlockHeader);
    mounted_ = true;
}

void FileSystem::unmount() {
    if (mounted_) {
        write_superblock();
        device_->sync();
        mounted_ = false;
        device_ = nullptr;
    }
}

void FileSystem::read_superblock() {
    std::vector<uint8_t> buf(device_->get_block_size());
    device_->read_block(0, buf.data());
    std::memcpy(&sb_, buf.data(), sizeof(SuperBlock));
}

void FileSystem::write_superblock() {
    std::vector<uint8_t> buf(sb_.block_size, 0);
    std::memcpy(buf.data(), &sb_, sizeof(SuperBlock));
    device_->write_block(0, buf.data());
}

bool FileSystem::get_bitmap_bit(uint32_t block_id) {
    if (block_id >= sb_.total_blocks) {
        throw std::out_of_range("Block id out of range in bitmap check");
    }
    uint32_t byte_idx = block_id / 8;
    uint32_t bit_idx = block_id % 8;
    uint32_t bmp_blk = sb_.bitmap_start + (byte_idx / sb_.block_size);
    uint32_t offset = byte_idx % sb_.block_size;

    std::vector<uint8_t> buf(sb_.block_size);
    device_->read_block(bmp_blk, buf.data());
    return (buf[offset] & (1 << bit_idx)) != 0;
}

void FileSystem::set_bitmap_bit(uint32_t block_id, bool used) {
    if (block_id >= sb_.total_blocks) {
        throw std::out_of_range("Block id out of range in bitmap set");
    }
    uint32_t byte_idx = block_id / 8;
    uint32_t bit_idx = block_id % 8;
    uint32_t bmp_blk = sb_.bitmap_start + (byte_idx / sb_.block_size);
    uint32_t offset = byte_idx % sb_.block_size;

    std::vector<uint8_t> buf(sb_.block_size);
    device_->read_block(bmp_blk, buf.data());
    if (used) {
        buf[offset] |= (1 << bit_idx);
    } else {
        buf[offset] &= ~(1 << bit_idx);
    }
    device_->write_block(bmp_blk, buf.data());
}

int32_t FileSystem::allocate_free_block() {
    if (sb_.free_blocks == 0) {
        return NO_BLOCK;
    }

    for (uint32_t b = sb_.data_start; b < sb_.total_blocks; ++b) {
        if (!get_bitmap_bit(b)) {
            set_bitmap_bit(b, true);
            sb_.free_blocks--;
            write_superblock();

            // Clear and initialize block header
            std::vector<uint8_t> block_buf(sb_.block_size, 0);
            BlockHeader* hdr = reinterpret_cast<BlockHeader*>(block_buf.data());
            hdr->next_block = NO_BLOCK;
            hdr->data_size = 0;
            hdr->flags = 0;
            device_->write_block(b, block_buf.data());

            return static_cast<int32_t>(b);
        }
    }
    return NO_BLOCK;
}

void FileSystem::free_block(uint32_t block_id) {
    if (block_id < sb_.data_start || block_id >= sb_.total_blocks) {
        throw std::out_of_range("Cannot free system or out-of-range block: " + std::to_string(block_id));
    }
    if (get_bitmap_bit(block_id)) {
        set_bitmap_bit(block_id, false);
        sb_.free_blocks++;
        write_superblock();
    }
}

void FileSystem::read_dir_entry(int entry_index, DirectoryEntry& entry) {
    uint32_t entries_per_block = sb_.block_size / sizeof(DirectoryEntry);
    uint32_t dir_blk = sb_.dir_start + (entry_index / entries_per_block);
    uint32_t offset = (entry_index % entries_per_block) * sizeof(DirectoryEntry);

    std::vector<uint8_t> buf(sb_.block_size);
    device_->read_block(dir_blk, buf.data());
    std::memcpy(&entry, buf.data() + offset, sizeof(DirectoryEntry));
}

void FileSystem::write_dir_entry(int entry_index, const DirectoryEntry& entry) {
    uint32_t entries_per_block = sb_.block_size / sizeof(DirectoryEntry);
    uint32_t dir_blk = sb_.dir_start + (entry_index / entries_per_block);
    uint32_t offset = (entry_index % entries_per_block) * sizeof(DirectoryEntry);

    std::vector<uint8_t> buf(sb_.block_size);
    device_->read_block(dir_blk, buf.data());
    std::memcpy(buf.data() + offset, &entry, sizeof(DirectoryEntry));
    device_->write_block(dir_blk, buf.data());
}

int FileSystem::find_dir_entry_index(const std::string& filename, DirectoryEntry& out_entry) {
    for (uint32_t i = 0; i < sb_.max_files; ++i) {
        read_dir_entry(i, out_entry);
        if (out_entry.is_used && filename == out_entry.name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int FileSystem::find_free_dir_entry_index() {
    DirectoryEntry entry{};
    for (uint32_t i = 0; i < sb_.max_files; ++i) {
        read_dir_entry(i, entry);
        if (!entry.is_used) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<int32_t> FileSystem::get_block_chain(int32_t first_block) {
    std::vector<int32_t> chain;
    if (first_block == NO_BLOCK) {
        return chain;
    }

    std::vector<uint8_t> buf(sb_.block_size);
    int32_t curr = first_block;
    uint32_t hops = 0;

    while (curr != NO_BLOCK) {
        if (curr < static_cast<int32_t>(sb_.data_start) || curr >= static_cast<int32_t>(sb_.total_blocks)) {
            throw std::runtime_error("Corrupted block pointer in chain: " + std::to_string(curr));
        }
        chain.push_back(curr);
        device_->read_block(curr, buf.data());
        const BlockHeader* hdr = reinterpret_cast<const BlockHeader*>(buf.data());
        curr = hdr->next_block;
        if (++hops > sb_.total_blocks) {
            throw std::runtime_error("Loop detected in block chain");
        }
    }
    return chain;
}

void FileSystem::free_block_chain(int32_t first_block) {
    if (first_block == NO_BLOCK) return;

    std::vector<uint8_t> buf(sb_.block_size);
    int32_t curr = first_block;
    uint32_t hops = 0;

    while (curr != NO_BLOCK) {
        if (curr < static_cast<int32_t>(sb_.data_start) || curr >= static_cast<int32_t>(sb_.total_blocks)) {
            break;
        }
        device_->read_block(curr, buf.data());
        const BlockHeader* hdr = reinterpret_cast<const BlockHeader*>(buf.data());
        int32_t next = hdr->next_block;
        free_block(curr);
        curr = next;
        if (++hops > sb_.total_blocks) {
            break;
        }
    }
}

void FileSystem::create_file(const std::string& filename) {
    if (!mounted_) throw std::runtime_error("Filesystem is not mounted");
    if (filename.empty()) throw std::invalid_argument("Filename cannot be empty");
    if (filename.length() > MAX_FILENAME_LEN) {
        throw std::invalid_argument("Filename exceeds maximum length (" + std::to_string(MAX_FILENAME_LEN) + ")");
    }
    if (filename.find('/') != std::string::npos) {
        throw std::invalid_argument("Subdirectories not supported in single-level filesystem: filename cannot contain '/'");
    }

    DirectoryEntry existing{};
    if (find_dir_entry_index(filename, existing) != -1) {
        throw std::runtime_error("File already exists: " + filename);
    }

    int free_idx = find_free_dir_entry_index();
    if (free_idx == -1) {
        throw std::runtime_error("Directory table is full (max files reached)");
    }

    DirectoryEntry entry{};
    std::strncpy(entry.name, filename.c_str(), sizeof(entry.name) - 1);
    entry.size = 0;
    entry.first_block = NO_BLOCK;
    entry.last_block = NO_BLOCK;
    entry.created_time = current_time_seconds();
    entry.modified_time = entry.created_time;
    entry.is_used = 1;
    entry.is_readonly = 0;

    write_dir_entry(free_idx, entry);
}

void FileSystem::delete_file(const std::string& filename) {
    if (!mounted_) throw std::runtime_error("Filesystem is not mounted");

    DirectoryEntry entry{};
    int idx = find_dir_entry_index(filename, entry);
    if (idx == -1) {
        throw std::runtime_error("File not found: " + filename);
    }

    if (entry.first_block != NO_BLOCK) {
        free_block_chain(entry.first_block);
    }

    // Clear entry
    std::memset(&entry, 0, sizeof(DirectoryEntry));
    write_dir_entry(idx, entry);
}

void FileSystem::copy_file(const std::string& src_name, const std::string& dst_name) {
    if (!mounted_) throw std::runtime_error("Filesystem is not mounted");

    std::vector<uint8_t> data;
    read_file(src_name, data);

    create_file(dst_name);
    if (!data.empty()) {
        write_file(dst_name, data.data(), data.size(), false);
    }
}

void FileSystem::move_file(const std::string& src_name, const std::string& dst_name) {
    // In single-level FS, move is renaming within root directory
    if (!mounted_) throw std::runtime_error("Filesystem is not mounted");
    if (dst_name.empty() || dst_name.length() > MAX_FILENAME_LEN) {
        throw std::invalid_argument("Invalid destination filename");
    }
    if (dst_name.find('/') != std::string::npos) {
        throw std::invalid_argument("Subdirectories not supported in single-level filesystem: destination cannot contain '/'");
    }

    DirectoryEntry src_entry{};
    int src_idx = find_dir_entry_index(src_name, src_entry);
    if (src_idx == -1) {
        throw std::runtime_error("Source file not found: " + src_name);
    }

    DirectoryEntry dst_entry{};
    if (find_dir_entry_index(dst_name, dst_entry) != -1) {
        throw std::runtime_error("Destination file already exists: " + dst_name);
    }

    std::memset(src_entry.name, 0, sizeof(src_entry.name));
    std::strncpy(src_entry.name, dst_name.c_str(), sizeof(src_entry.name) - 1);
    src_entry.modified_time = current_time_seconds();

    write_dir_entry(src_idx, src_entry);
}

size_t FileSystem::write_file(const std::string& filename, const void* data, size_t size, bool append) {
    if (!mounted_) throw std::runtime_error("Filesystem is not mounted");

    DirectoryEntry entry{};
    int entry_idx = find_dir_entry_index(filename, entry);
    if (entry_idx == -1) {
        throw std::runtime_error("File not found: " + filename);
    }

    if (!append) {
        // Overwrite: clear existing blocks
        if (entry.first_block != NO_BLOCK) {
            free_block_chain(entry.first_block);
            entry.first_block = NO_BLOCK;
            entry.last_block = NO_BLOCK;
            entry.size = 0;
        }
    }

    if (size == 0) {
        entry.modified_time = current_time_seconds();
        write_dir_entry(entry_idx, entry);
        return 0;
    }

    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    size_t remaining = size;

    // Check if we can append into the existing last block
    if (entry.last_block != NO_BLOCK) {
        std::vector<uint8_t> last_buf(sb_.block_size);
        device_->read_block(entry.last_block, last_buf.data());
        BlockHeader* hdr = reinterpret_cast<BlockHeader*>(last_buf.data());

        uint32_t available = payload_capacity_ - hdr->data_size;
        if (available > 0) {
            uint32_t to_copy = std::min(static_cast<uint32_t>(remaining), available);
            std::memcpy(last_buf.data() + sizeof(BlockHeader) + hdr->data_size, ptr, to_copy);
            hdr->data_size += to_copy;
            device_->write_block(entry.last_block, last_buf.data());

            ptr += to_copy;
            remaining -= to_copy;
            entry.size += to_copy;
        }
    }

    // Allocate new blocks for remaining data
    while (remaining > 0) {
        int32_t new_block = allocate_free_block();
        if (new_block == NO_BLOCK) {
            // Out of disk space
            entry.modified_time = current_time_seconds();
            write_dir_entry(entry_idx, entry);
            throw std::runtime_error("Disk is full: failed to allocate data block");
        }

        // Link previous block to new_block
        if (entry.last_block != NO_BLOCK) {
            std::vector<uint8_t> prev_buf(sb_.block_size);
            device_->read_block(entry.last_block, prev_buf.data());
            BlockHeader* prev_hdr = reinterpret_cast<BlockHeader*>(prev_buf.data());
            prev_hdr->next_block = new_block;
            device_->write_block(entry.last_block, prev_buf.data());
        } else {
            entry.first_block = new_block;
        }
        entry.last_block = new_block;

        // Fill new block
        std::vector<uint8_t> new_buf(sb_.block_size, 0);
        BlockHeader* new_hdr = reinterpret_cast<BlockHeader*>(new_buf.data());
        uint32_t to_copy = std::min(static_cast<uint32_t>(remaining), payload_capacity_);
        new_hdr->next_block = NO_BLOCK;
        new_hdr->data_size = to_copy;
        std::memcpy(new_buf.data() + sizeof(BlockHeader), ptr, to_copy);
        device_->write_block(new_block, new_buf.data());

        ptr += to_copy;
        remaining -= to_copy;
        entry.size += to_copy;
    }

    entry.modified_time = current_time_seconds();
    write_dir_entry(entry_idx, entry);
    return size;
}

size_t FileSystem::read_file(const std::string& filename, std::vector<uint8_t>& buffer) {
    if (!mounted_) throw std::runtime_error("Filesystem is not mounted");

    DirectoryEntry entry{};
    int entry_idx = find_dir_entry_index(filename, entry);
    if (entry_idx == -1) {
        throw std::runtime_error("File not found: " + filename);
    }

    buffer.clear();
    buffer.reserve(entry.size);

    if (entry.size == 0 || entry.first_block == NO_BLOCK) {
        return 0;
    }

    int32_t curr = entry.first_block;
    uint32_t remaining = entry.size;
    std::vector<uint8_t> block_buf(sb_.block_size);

    while (curr != NO_BLOCK && remaining > 0) {
        device_->read_block(curr, block_buf.data());
        const BlockHeader* hdr = reinterpret_cast<const BlockHeader*>(block_buf.data());
        uint32_t chunk = std::min(static_cast<uint32_t>(hdr->data_size), remaining);

        const uint8_t* payload = block_buf.data() + sizeof(BlockHeader);
        buffer.insert(buffer.end(), payload, payload + chunk);
        remaining -= chunk;
        curr = hdr->next_block;
    }

    return buffer.size();
}

size_t FileSystem::read_range(const std::string& filename, size_t offset, size_t length, std::vector<uint8_t>& buffer, size_t& blocks_traversed) {
    if (!mounted_) throw std::runtime_error("Filesystem is not mounted");

    DirectoryEntry entry{};
    int entry_idx = find_dir_entry_index(filename, entry);
    if (entry_idx == -1) {
        throw std::runtime_error("File not found: " + filename);
    }

    buffer.clear();
    blocks_traversed = 0;

    if (offset >= entry.size || length == 0) {
        return 0;
    }

    size_t actual_len = std::min(length, static_cast<size_t>(entry.size) - offset);
    buffer.reserve(actual_len);

    int32_t curr = entry.first_block;
    size_t current_file_pos = 0;
    std::vector<uint8_t> block_buf(sb_.block_size);

    while (curr != NO_BLOCK && buffer.size() < actual_len) {
        ++blocks_traversed;
        device_->read_block(curr, block_buf.data());
        const BlockHeader* hdr = reinterpret_cast<const BlockHeader*>(block_buf.data());
        size_t block_data_len = hdr->data_size;

        size_t block_start = current_file_pos;
        size_t block_end = current_file_pos + block_data_len;

        if (offset < block_end && (offset + actual_len) > block_start) {
            size_t read_start = (offset > block_start) ? (offset - block_start) : 0;
            size_t read_end = std::min(block_data_len, (offset + actual_len) - block_start);
            size_t bytes_to_copy = read_end - read_start;

            const uint8_t* payload = block_buf.data() + sizeof(BlockHeader) + read_start;
            buffer.insert(buffer.end(), payload, payload + bytes_to_copy);
        }

        current_file_pos += block_data_len;
        curr = hdr->next_block;
    }

    return buffer.size();
}

std::vector<FileInfo> FileSystem::list_files() {
    if (!mounted_) throw std::runtime_error("Filesystem is not mounted");

    std::vector<FileInfo> list;
    DirectoryEntry entry{};
    for (uint32_t i = 0; i < sb_.max_files; ++i) {
        read_dir_entry(i, entry);
        if (entry.is_used) {
            FileInfo info;
            info.name = entry.name;
            info.size = entry.size;
            info.first_block = entry.first_block;
            info.last_block = entry.last_block;
            info.created_time = entry.created_time;
            info.modified_time = entry.modified_time;
            info.is_readonly = (entry.is_readonly != 0);
            info.block_chain = get_block_chain(entry.first_block);
            info.blocks_count = static_cast<uint32_t>(info.block_chain.size());
            list.push_back(info);
        }
    }
    return list;
}

FileInfo FileSystem::get_file_info(const std::string& filename) {
    if (!mounted_) throw std::runtime_error("Filesystem is not mounted");

    DirectoryEntry entry{};
    int idx = find_dir_entry_index(filename, entry);
    if (idx == -1) {
        throw std::runtime_error("File not found: " + filename);
    }

    FileInfo info;
    info.name = entry.name;
    info.size = entry.size;
    info.first_block = entry.first_block;
    info.last_block = entry.last_block;
    info.created_time = entry.created_time;
    info.modified_time = entry.modified_time;
    info.is_readonly = (entry.is_readonly != 0);
    info.block_chain = get_block_chain(entry.first_block);
    info.blocks_count = static_cast<uint32_t>(info.block_chain.size());
    return info;
}

bool FileSystem::file_exists(const std::string& filename) {
    if (!mounted_) return false;
    DirectoryEntry entry{};
    return find_dir_entry_index(filename, entry) != -1;
}

FragmentationStats FileSystem::get_fragmentation_stats() {
    if (!mounted_) throw std::runtime_error("Filesystem is not mounted");

    auto files = list_files();
    FragmentationStats stats{};
    stats.total_files = static_cast<uint32_t>(files.size());

    uint64_t total_distance = 0;
    uint32_t fragmented_hops = 0;

    for (const auto& f : files) {
        stats.total_allocated_blocks += f.blocks_count;
        if (f.block_chain.size() > 1) {
            for (size_t i = 0; i < f.block_chain.size() - 1; ++i) {
                int32_t b1 = f.block_chain[i];
                int32_t b2 = f.block_chain[i + 1];
                int32_t dist = std::abs(b2 - b1);
                total_distance += dist;
                stats.total_hops++;
                if (dist != 1) {
                    fragmented_hops++;
                }
            }
        }
    }

    if (stats.total_hops > 0) {
        stats.avg_hop_distance = static_cast<double>(total_distance) / stats.total_hops;
        stats.fragmentation_ratio = (static_cast<double>(fragmented_hops) / stats.total_hops) * 100.0;
    } else {
        stats.avg_hop_distance = 1.0;
        stats.fragmentation_ratio = 0.0;
    }

    return stats;
}

void FileSystem::dump(std::ostream& os, bool include_block_contents, const std::string& file_filter) {
    if (!mounted_) {
        os << "[ERROR] Filesystem is not mounted.\n";
        return;
    }

    os << "================================================================================\n";
    os << "                 FILESYSTEM DUMP (VARIANT 2: LINKED LIST OF BLOCKS)             \n";
    os << "================================================================================\n\n";

    // 1. Superblock Information
    os << "--- 1. SUPERBLOCK INFO ---\n";
    os << "Magic Signature:      0x" << std::hex << sb_.magic << std::dec 
       << (sb_.magic == FS_MAGIC ? " (VALID - 'LLFS')" : " (INVALID!)") << "\n";
    os << "Block Size:           " << sb_.block_size << " bytes\n";
    os << "Total Blocks:         " << sb_.total_blocks << " (" 
       << (static_cast<uint64_t>(sb_.total_blocks) * sb_.block_size) / 1024 << " KB)\n";
    os << "Free Data Blocks:     " << sb_.free_blocks << "\n";
    os << "Used Data Blocks:     " << (sb_.total_blocks - sb_.data_start - sb_.free_blocks) << "\n";
    os << "System Blocks:        " << sb_.data_start << " (Blocks 0.." << sb_.data_start - 1 << ")\n";
    os << "  - Superblock:       Block 0\n";
    os << "  - Bitmap Range:     Blocks " << sb_.bitmap_start << ".." << (sb_.bitmap_start + sb_.bitmap_blocks - 1) 
       << " (" << sb_.bitmap_blocks << " blocks)\n";
    os << "  - Directory Range:  Blocks " << sb_.dir_start << ".." << (sb_.dir_start + sb_.dir_blocks - 1) 
       << " (" << sb_.dir_blocks << " blocks, capacity: " << sb_.max_files << " files)\n";
    os << "Data Blocks Start:    Block " << sb_.data_start << "\n";
    os << "Payload per Block:    " << payload_capacity_ << " bytes (Header: " << sizeof(BlockHeader) << " bytes)\n\n";

    // 2. Free Block Bitmap Visualization
    os << "--- 2. BLOCK ALLOCATION BITMAP ---\n";
    os << "Legend: [S] = System metadata, [#] = Allocated data block, [.] = Free block\n";
    uint32_t cols = 64;
    for (uint32_t b = 0; b < sb_.total_blocks; ++b) {
        if (b % cols == 0) {
            os << std::right << std::dec << std::setw(6) << std::setfill('0') << b << ": ";
        }
        if (b < sb_.data_start) {
            os << 'S';
        } else if (get_bitmap_bit(b)) {
            os << '#';
        } else {
            os << '.';
        }
        if ((b + 1) % cols == 0 || b + 1 == sb_.total_blocks) {
            os << "\n";
        }
    }
    os << std::setfill(' ') << "\n";

    // 3. Single-Level Directory Listing & Linked Block Chains
    os << "--- 3. DIRECTORY & FILE BLOCK CHAINS ---\n";
    auto files = list_files();
    if (files.empty()) {
        os << "(Directory is empty)\n\n";
    } else {
        os << std::left 
           << std::setw(24) << "Filename" 
           << std::setw(12) << "Size (B)" 
           << std::setw(10) << "Blocks" 
           << std::setw(12) << "1st Block" 
           << std::setw(12) << "Last Block" 
           << "Linked List Block Chain\n";
        os << std::string(80, '-') << "\n";

        for (const auto& f : files) {
            if (!file_filter.empty() && f.name != file_filter) {
                continue;
            }
            os << std::left 
               << std::setw(24) << f.name 
               << std::setw(12) << f.size 
               << std::setw(10) << f.blocks_count 
               << std::setw(12) << (f.first_block == NO_BLOCK ? "NONE" : std::to_string(f.first_block))
               << std::setw(12) << (f.last_block == NO_BLOCK ? "NONE" : std::to_string(f.last_block));

            if (f.block_chain.empty()) {
                os << "EMPTY (no blocks allocated)\n";
            } else {
                for (size_t i = 0; i < f.block_chain.size(); ++i) {
                    os << "[" << f.block_chain[i] << "] -> ";
                }
                os << "EOF (-1)\n";
            }
        }
        os << "\n";
    }

    // 4. Block Contents (Detailed Hex/ASCII Dump)
    if (include_block_contents && !files.empty()) {
        os << "--- 4. DATA BLOCK CONTENTS (HEX / ASCII) ---\n";
        std::vector<uint8_t> block_buf(sb_.block_size);

        for (const auto& f : files) {
            if (!file_filter.empty() && f.name != file_filter) {
                continue;
            }
            os << ">>> File: " << f.name << " (Size: " << f.size << " bytes, " << f.blocks_count << " blocks)\n";
            if (f.block_chain.empty()) {
                os << "    [Empty File]\n";
                continue;
            }

            for (size_t i = 0; i < f.block_chain.size(); ++i) {
                int32_t blk_id = f.block_chain[i];
                device_->read_block(blk_id, block_buf.data());
                const BlockHeader* hdr = reinterpret_cast<const BlockHeader*>(block_buf.data());

                os << "  + Block #" << blk_id << " (Chain Index: " << i << ")\n";
                os << "    Header: [next_block = " << hdr->next_block 
                   << (hdr->next_block == NO_BLOCK ? " (EOF)" : "") 
                   << ", data_size = " << hdr->data_size << " bytes]\n";

                const uint8_t* payload = block_buf.data() + sizeof(BlockHeader);
                uint32_t len = hdr->data_size;

                // Hex dump of payload (16 bytes per line)
                for (uint32_t offset = 0; offset < len; offset += 16) {
                    os << "    " << std::right << std::setw(4) << std::setfill('0') << std::hex << offset << ": ";
                    uint32_t line_len = std::min(16u, len - offset);

                    for (uint32_t j = 0; j < 16; ++j) {
                        if (j < line_len) {
                            os << std::right << std::setw(2) << std::setfill('0') << static_cast<int>(payload[offset + j]) << " ";
                        } else {
                            os << "   ";
                        }
                        if (j == 7) os << " ";
                    }
                    os << " |";
                    for (uint32_t j = 0; j < line_len; ++j) {
                        uint8_t c = payload[offset + j];
                        os << (std::isprint(c) ? static_cast<char>(c) : '.');
                    }
                    os << "|\n";
                }
                os << std::dec << std::setfill(' ');
            }
            os << "\n";
        }
    }

    // 5. Fragmentation Summary
    auto frag = get_fragmentation_stats();
    os << "--- 5. FRAGMENTATION METRICS ---\n";
    os << "Total Files:          " << frag.total_files << "\n";
    os << "Total Data Blocks:    " << frag.total_allocated_blocks << "\n";
    os << "Total Block Hops:     " << frag.total_hops << "\n";
    os << "Average Hop Distance: " << std::fixed << std::setprecision(2) << frag.avg_hop_distance << " blocks\n";
    os << "Fragmentation Ratio:  " << std::fixed << std::setprecision(1) << frag.fragmentation_ratio << " %\n";
    os << "================================================================================\n";
}

} // namespace fs
