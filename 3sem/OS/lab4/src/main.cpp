#include "block_device.hpp"
#include "file_system.hpp"
#include "cli.hpp"
#include <iostream>
#include <memory>
#include <filesystem>

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "Options:\n"
              << "  --image <file>        Path to disk image (default: virtual_disk.bin)\n"
              << "  --mem                 Use in-memory storage (non-persistent)\n"
              << "  --format              Format disk on startup\n"
              << "  --blocks <N>          Total blocks for format (default: 2048)\n"
              << "  --block-size <B>      Block size in bytes (default: 512)\n"
              << "  --script <file>       Execute script commands from file and exit\n"
              << "  --exec \"<command>\"    Execute a single command and exit\n"
              << "  --help                Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string image_path = "virtual_disk.bin";
    bool use_mem = false;
    bool force_format = false;
    uint32_t total_blocks = fs::DEFAULT_TOTAL_BLOCKS;
    uint32_t block_size = fs::DEFAULT_BLOCK_SIZE;
    std::string script_path;
    std::string exec_command;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--image" && i + 1 < argc) {
            image_path = argv[++i];
        } else if (arg == "--mem") {
            use_mem = true;
        } else if (arg == "--format") {
            force_format = true;
        } else if (arg == "--blocks" && i + 1 < argc) {
            total_blocks = std::stoul(argv[++i]);
        } else if (arg == "--block-size" && i + 1 < argc) {
            block_size = std::stoul(argv[++i]);
        } else if (arg == "--script" && i + 1 < argc) {
            script_path = argv[++i];
        } else if (arg == "--exec" && i + 1 < argc) {
            exec_command = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    try {
        std::shared_ptr<fs::BlockDevice> device;
        bool needs_format = force_format;

        if (use_mem) {
            device = std::make_shared<fs::MemoryBlockDevice>(block_size, total_blocks);
            needs_format = true;
        } else {
            bool exists = std::filesystem::exists(image_path);
            if (!exists) {
                needs_format = true;
            }
            device = std::make_shared<fs::FileBlockDevice>(image_path, block_size, total_blocks, true);
        }

        if (needs_format) {
            std::cout << "Formatting virtual disk (" << total_blocks << " blocks x " << block_size << " bytes)...\n";
            fs::FileSystem::format(*device);
        }

        auto file_system = std::make_shared<fs::FileSystem>();
        file_system->mount(device);

        fs::CLI cli(file_system);

        if (!script_path.empty()) {
            cli.execute_script(script_path);
        } else if (!exec_command.empty()) {
            cli.execute_command(exec_command);
        } else {
            cli.run_interactive();
        }

        file_system->unmount();
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
