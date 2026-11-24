#include "cli.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <ctime>

namespace fs {

CLI::CLI(std::shared_ptr<FileSystem> fs) : fs_(std::move(fs)) {}

std::vector<std::string> CLI::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    bool in_quotes = false;
    char quote_char = '"';

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == quote_char) {
                in_quotes = false;
            } else {
                token += c;
            }
        } else {
            if (c == '"' || c == '\'') {
                in_quotes = true;
                quote_char = c;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

void CLI::print_help() {
    std::cout << "\n============================= AVAILABLE COMMANDS =============================\n"
              << "  help                                - Show this help menu\n"
              << "  format [blocks] [block_size]        - Format the virtual filesystem\n"
              << "  create <file>                       - Create a new empty file\n"
              << "  touch <file>                        - Alias for create\n"
              << "  rm <file>                           - Delete a file (free its block chain)\n"
              << "  cp <src> <dest>                     - Copy file src to dest\n"
              << "  mv <src> <dest>                     - Move/rename file src to dest\n"
              << "  write <file> <text...>              - Overwrite file with text\n"
              << "  append <file> <text...>             - Append text to file\n"
              << "  read <file>                         - Read and display file contents\n"
              << "  cat <file>                          - Alias for read\n"
              << "  read_range <file> <offset> <length> - Read slice of file & show block hops\n"
              << "  ls                                  - List all files in the directory\n"
              << "  stat <file>                         - Show file details and full block chain\n"
              << "  import <host_file> <fs_file>        - Import host file into virtual FS\n"
              << "  export <fs_file> <host_file>        - Export virtual FS file to host\n"
              << "  dump [output_filepath]              - Full FS dump (structure, chains, hex)\n"
              << "  stats                               - Device I/O counters & fragmentation\n"
              << "  bench                               - Run quick internal performance demo\n"
              << "  exit / quit                         - Exit the filesystem shell\n"
              << "==============================================================================\n\n";
}

bool CLI::execute_command(const std::string& line) {
    auto tokens = tokenize(line);
    if (tokens.empty()) return true;

    const std::string& cmd = tokens[0];

    try {
        if (cmd == "help") {
            print_help();
        } else if (cmd == "exit" || cmd == "quit") {
            running_ = false;
            return false;
        } else if (cmd == "format") {
            cmd_format(tokens);
        } else if (cmd == "create" || cmd == "touch") {
            cmd_create(tokens);
        } else if (cmd == "rm" || cmd == "del" || cmd == "delete") {
            cmd_delete(tokens);
        } else if (cmd == "cp" || cmd == "copy") {
            cmd_copy(tokens);
        } else if (cmd == "mv" || cmd == "move" || cmd == "rename") {
            cmd_move(tokens);
        } else if (cmd == "write") {
            cmd_write(tokens, false);
        } else if (cmd == "append") {
            cmd_write(tokens, true);
        } else if (cmd == "read" || cmd == "cat") {
            cmd_read(tokens);
        } else if (cmd == "read_range") {
            cmd_read_range(tokens);
        } else if (cmd == "ls" || cmd == "dir") {
            cmd_ls(tokens);
        } else if (cmd == "stat") {
            cmd_stat(tokens);
        } else if (cmd == "import") {
            cmd_import(tokens);
        } else if (cmd == "export") {
            cmd_export(tokens);
        } else if (cmd == "dump") {
            cmd_dump(tokens);
        } else if (cmd == "stats") {
            cmd_stats(tokens);
        } else if (cmd == "bench") {
            cmd_bench(tokens);
        } else {
            std::cout << "[ERROR] Unknown command: " << cmd << ". Type 'help' for command list.\n";
        }
    } catch (const std::exception& e) {
        std::cout << "[ERROR] " << e.what() << "\n";
    }

    return true;
}

void CLI::run_interactive() {
    std::cout << "==================================================================\n";
    std::cout << "  Single-Level FileSystem Shell (Linked List of Blocks Model)\n";
    std::cout << "  Laboratory Work #4 (Variant 2) | Type 'help' for commands\n";
    std::cout << "==================================================================\n";

    std::string line;
    while (running_) {
        std::cout << "fs> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        execute_command(line);
    }
    std::cout << "Exiting filesystem shell.\n";
}

bool CLI::execute_script(const std::string& script_filepath) {
    std::ifstream in(script_filepath);
    if (!in.is_open()) {
        std::cerr << "[ERROR] Cannot open script file: " << script_filepath << "\n";
        return false;
    }

    std::string line;
    int line_num = 0;
    while (std::getline(in, line)) {
        line_num++;
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        std::cout << "fs [" << line_num << "]> " << line << "\n";
        if (!execute_command(line)) {
            break;
        }
    }
    return true;
}

void CLI::cmd_format(const std::vector<std::string>& args) {
    auto dev = fs_->get_device();
    if (!dev) {
        std::cout << "[ERROR] No device attached.\n";
        return;
    }
    uint32_t max_files = DEFAULT_MAX_FILES;
    if (args.size() >= 2) {
        max_files = std::stoul(args[1]);
    }
    fs_->unmount();
    FileSystem::format(*dev, max_files);
    fs_->mount(dev);
    std::cout << "[OK] Filesystem formatted successfully (Max files: " << max_files << ").\n";
}

void CLI::cmd_create(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: create <filename>\n";
        return;
    }
    fs_->create_file(args[1]);
    std::cout << "[OK] File created: " << args[1] << "\n";
}

void CLI::cmd_delete(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: rm <filename>\n";
        return;
    }
    fs_->delete_file(args[1]);
    std::cout << "[OK] File deleted: " << args[1] << "\n";
}

void CLI::cmd_copy(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cout << "Usage: cp <source> <dest>\n";
        return;
    }
    fs_->copy_file(args[1], args[2]);
    std::cout << "[OK] Copied " << args[1] << " -> " << args[2] << "\n";
}

void CLI::cmd_move(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cout << "Usage: mv <source> <dest>\n";
        return;
    }
    fs_->move_file(args[1], args[2]);
    std::cout << "[OK] Moved (renamed) " << args[1] << " -> " << args[2] << "\n";
}

void CLI::cmd_write(const std::vector<std::string>& args, bool append) {
    if (args.size() < 3) {
        std::cout << "Usage: " << (append ? "append" : "write") << " <filename> <text...>\n";
        return;
    }

    std::string text;
    for (size_t i = 2; i < args.size(); ++i) {
        if (i > 2) text += " ";
        text += args[i];
    }

    size_t written = fs_->write_file(args[1], text.data(), text.size(), append);
    std::cout << "[OK] " << (append ? "Appended " : "Written ") << written 
              << " bytes to file " << args[1] << ".\n";
}

void CLI::cmd_read(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: read <filename>\n";
        return;
    }
    std::vector<uint8_t> data;
    fs_->read_file(args[1], data);
    std::cout << "--- Contents of " << args[1] << " (" << data.size() << " bytes) ---\n";
    std::cout.write(reinterpret_cast<const char*>(data.data()), data.size());
    std::cout << "\n----------------------------------------\n";
}

void CLI::cmd_read_range(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        std::cout << "Usage: read_range <filename> <offset> <length>\n";
        return;
    }
    size_t offset = std::stoull(args[2]);
    size_t length = std::stoull(args[3]);
    std::vector<uint8_t> data;
    size_t hops = 0;

    fs_->read_range(args[1], offset, length, data, hops);
    std::cout << "[OK] Read " << data.size() << " bytes (offset " << offset 
              << ", requested " << length << "). Block hops traversed: " << hops << ".\n";
    std::cout << "--- Data ---\n";
    std::cout.write(reinterpret_cast<const char*>(data.data()), data.size());
    std::cout << "\n------------\n";
}

void CLI::cmd_ls(const std::vector<std::string>& /*args*/) {
    auto files = fs_->list_files();
    if (files.empty()) {
        std::cout << "(Empty directory)\n";
        return;
    }

    std::cout << std::left 
              << std::setw(24) << "Filename" 
              << std::setw(12) << "Size (bytes)" 
              << std::setw(10) << "Blocks" 
              << std::setw(14) << "1st Block" 
              << "Created (UTC)\n";
    std::cout << std::string(75, '-') << "\n";

    for (const auto& f : files) {
        std::time_t t = static_cast<std::time_t>(f.created_time);
        char time_buf[64];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::gmtime(&t));

        std::cout << std::left 
                  << std::setw(24) << f.name 
                  << std::setw(12) << f.size 
                  << std::setw(10) << f.blocks_count 
                  << std::setw(14) << (f.first_block == NO_BLOCK ? "-" : std::to_string(f.first_block))
                  << time_buf << "\n";
    }
}

void CLI::cmd_stat(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: stat <filename>\n";
        return;
    }
    auto f = fs_->get_file_info(args[1]);
    std::cout << "File:            " << f.name << "\n";
    std::cout << "Size:            " << f.size << " bytes\n";
    std::cout << "Allocated blocks:" << f.blocks_count << "\n";
    std::cout << "First Block:     " << (f.first_block == NO_BLOCK ? "NONE" : std::to_string(f.first_block)) << "\n";
    std::cout << "Last Block:      " << (f.last_block == NO_BLOCK ? "NONE" : std::to_string(f.last_block)) << "\n";
    std::cout << "Linked Chain:    ";
    if (f.block_chain.empty()) {
        std::cout << "NONE (empty file)\n";
    } else {
        for (size_t i = 0; i < f.block_chain.size(); ++i) {
            std::cout << "[" << f.block_chain[i] << "] -> ";
        }
        std::cout << "EOF (-1)\n";
    }
}

void CLI::cmd_import(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cout << "Usage: import <host_file> <fs_filename>\n";
        return;
    }
    std::ifstream in(args[1], std::ios::binary);
    if (!in.is_open()) {
        std::cout << "[ERROR] Cannot open host file: " << args[1] << "\n";
        return;
    }
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (!fs_->file_exists(args[2])) {
        fs_->create_file(args[2]);
    }
    fs_->write_file(args[2], buffer.data(), buffer.size(), false);
    std::cout << "[OK] Imported " << buffer.size() << " bytes from host file '" 
              << args[1] << "' into '" << args[2] << "'.\n";
}

void CLI::cmd_export(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cout << "Usage: export <fs_filename> <host_file>\n";
        return;
    }
    std::vector<uint8_t> buffer;
    fs_->read_file(args[1], buffer);
    std::ofstream out(args[2], std::ios::binary);
    if (!out.is_open()) {
        std::cout << "[ERROR] Cannot open host destination file: " << args[2] << "\n";
        return;
    }
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    std::cout << "[OK] Exported " << buffer.size() << " bytes from '" 
              << args[1] << "' to host file '" << args[2] << "'.\n";
}

void CLI::cmd_dump(const std::vector<std::string>& args) {
    if (args.size() >= 2) {
        std::ofstream out(args[1]);
        if (!out.is_open()) {
            std::cout << "[ERROR] Cannot open dump output file: " << args[1] << "\n";
            return;
        }
        fs_->dump(out, true);
        std::cout << "[OK] Filesystem dump written to file: " << args[1] << "\n";
    } else {
        fs_->dump(std::cout, true);
    }
}

void CLI::cmd_stats(const std::vector<std::string>& /*args*/) {
    auto dev = fs_->get_device();
    std::cout << "--- Device I/O Statistics ---\n";
    if (dev) {
        std::cout << "Read operations:  " << dev->get_read_ops() << " blocks\n";
        std::cout << "Write operations: " << dev->get_write_ops() << " blocks\n";
    }
    auto frag = fs_->get_fragmentation_stats();
    std::cout << "--- Fragmentation Metrics ---\n";
    std::cout << "Total Files:          " << frag.total_files << "\n";
    std::cout << "Total Data Blocks:    " << frag.total_allocated_blocks << "\n";
    std::cout << "Average Hop Distance: " << std::fixed << std::setprecision(2) << frag.avg_hop_distance << " blocks\n";
    std::cout << "Fragmentation Ratio:  " << std::fixed << std::setprecision(1) << frag.fragmentation_ratio << " %\n";
}

void CLI::cmd_bench(const std::vector<std::string>& /*args*/) {
    std::cout << "Running quick demonstration benchmark...\n";
    // Quick in-memory benchmark test
    const std::string test_file = "bench_demo.bin";
    if (fs_->file_exists(test_file)) {
        fs_->delete_file(test_file);
    }
    fs_->create_file(test_file);

    std::vector<uint8_t> sample_data(10 * 504, 0xAB); // 10 blocks
    auto t0 = std::chrono::high_resolution_clock::now();
    fs_->write_file(test_file, sample_data.data(), sample_data.size(), false);
    auto t1 = std::chrono::high_resolution_clock::now();

    std::vector<uint8_t> read_buf;
    auto t2 = std::chrono::high_resolution_clock::now();
    fs_->read_file(test_file, read_buf);
    auto t3 = std::chrono::high_resolution_clock::now();

    size_t hops = 0;
    std::vector<uint8_t> slice_buf;
    auto t4 = std::chrono::high_resolution_clock::now();
    fs_->read_range(test_file, 9 * 504, 100, slice_buf, hops);
    auto t5 = std::chrono::high_resolution_clock::now();

    auto write_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    auto read_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();
    auto seek_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t5 - t4).count();

    std::cout << "  Write 10 blocks (5040 B): " << write_ns << " ns\n";
    std::cout << "  Read 10 blocks (5040 B):  " << read_ns << " ns\n";
    std::cout << "  Seek & read 10th block:   " << seek_ns << " ns (traversed " << hops << " blocks)\n";
    std::cout << "Benchmark demo completed.\n";
}

} // namespace fs
