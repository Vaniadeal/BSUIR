#pragma once

#include "file_system.hpp"
#include <string>
#include <vector>
#include <memory>

namespace fs {

class CLI {
public:
    explicit CLI(std::shared_ptr<FileSystem> fs);
    ~CLI() = default;

    void run_interactive();
    bool execute_command(const std::string& line);
    bool execute_script(const std::string& script_filepath);

private:
    std::shared_ptr<FileSystem> fs_;
    bool running_{true};

    std::vector<std::string> tokenize(const std::string& line);
    void print_help();
    void cmd_format(const std::vector<std::string>& args);
    void cmd_create(const std::vector<std::string>& args);
    void cmd_delete(const std::vector<std::string>& args);
    void cmd_copy(const std::vector<std::string>& args);
    void cmd_move(const std::vector<std::string>& args);
    void cmd_write(const std::vector<std::string>& args, bool append);
    void cmd_read(const std::vector<std::string>& args);
    void cmd_read_range(const std::vector<std::string>& args);
    void cmd_ls(const std::vector<std::string>& args);
    void cmd_stat(const std::vector<std::string>& args);
    void cmd_import(const std::vector<std::string>& args);
    void cmd_export(const std::vector<std::string>& args);
    void cmd_dump(const std::vector<std::string>& args);
    void cmd_stats(const std::vector<std::string>& args);
    void cmd_bench(const std::vector<std::string>& args);
};

} // namespace fs
