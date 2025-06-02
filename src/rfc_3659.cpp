#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>

#include "macros.h"

static std::string format_file_time(const std::filesystem::file_time_type& l_time_point) {

    auto file_time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(l_time_point - std::filesystem::file_time_type::clock::now());

    std::time_t file_time_t = std::chrono::system_clock::to_time_t(file_time);

    tm* tm = std::gmtime(&file_time_t);

    char buffer[20];

    std::strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", tm);

    return buffer;
}

//===========================================================

std::string mdtm(const std::string& l_path) {

    FSTRY

    return "213 " + format_file_time(std::filesystem::last_write_time(l_path)) + "\r\n";

    FSCATCH

    return "550 File not found";
}

std::string mlst(const std::string& l_path) {

    std::string file_name = l_path;

    if (file_name[file_name.length() - 1] == '/' && file_name.length() > 1) {
        file_name.pop_back();
    }

    file_name = file_name.substr(file_name.rfind('/') + 1, file_name.length());

    if (std::filesystem::is_directory(l_path)) {
        return "Size=0" + (";Modify=" + format_file_time(std::filesystem::last_write_time(l_path)) + ";Type=dir; " + file_name) + "\r\n\r\n";
    } else {
        return "Size=" + std::to_string(std::filesystem::file_size(l_path)) + ";Modify=" + format_file_time(std::filesystem::last_write_time(l_path)) + ";Type=file; " + file_name + "\r\n\r\n";
    }
}

std::string mlsd(const std::string& l_path) {

    std::string names;

    FSTRY

    for (const auto& entry : std::filesystem::directory_iterator(l_path)) {

        if (entry.is_directory()) {
            names += "Size=0;Modify=" + format_file_time(entry.last_write_time()) + ";Type=dir; " + entry.path().filename().string();
        } else {
            names += "Size=" + std::to_string(entry.file_size()) + ";Modify=" + format_file_time(entry.last_write_time()) + ";Type=file; " + entry.path().filename().string();
        }
        names += "\r\n";
    }

    FSCATCH

    return names;
}

int getSize(const std::string& l_path) {

    FSTRY
    return std::filesystem::file_size(l_path);
    FSCATCH
    return -1;
}
