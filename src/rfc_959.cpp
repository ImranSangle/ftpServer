#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "log.h"
#include "macros.h"
#include "sockets.h"

static std::string get_permissions(const std::filesystem::perms& l_perms) {

    auto get_perm_lmda = [=](char value, const std::filesystem::perms& c_perms) {
        return std::filesystem::perms::none == (c_perms & l_perms) ? '-' : value;
    };

    std::string permissions;

    permissions += get_perm_lmda('r', std::filesystem::perms::owner_read);
    permissions += get_perm_lmda('w', std::filesystem::perms::owner_write);
    permissions += get_perm_lmda('x', std::filesystem::perms::owner_exec);
    permissions += get_perm_lmda('r', std::filesystem::perms::group_read);
    permissions += get_perm_lmda('w', std::filesystem::perms::group_write);
    permissions += get_perm_lmda('x', std::filesystem::perms::group_exec);
    permissions += get_perm_lmda('r', std::filesystem::perms::others_read);
    permissions += get_perm_lmda('w', std::filesystem::perms::others_write);
    permissions += get_perm_lmda('x', std::filesystem::perms::others_exec);
    permissions += " ";

    return permissions;
}

static std::string format_file_time(const std::filesystem::file_time_type& l_time_point) {

    auto file_time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(l_time_point - std::filesystem::file_time_type::clock::now());

    std::time_t file_time_t = std::chrono::system_clock::to_time_t(file_time);

    std::ostringstream oss;

    oss << std::put_time(std::localtime(&file_time_t), " %b %d %H:%M ");

    return oss.str();
}

//========================================================

std::string nlst(const std::string& l_path) {

    std::string names;

    FSTRY

    for (const auto& entry : std::filesystem::directory_iterator(l_path)) {

        names += entry.path().filename().string();
        names += "\r\n";
    }

    FSCATCH

    return names;
}

std::string list(const std::string& l_path) {

    std::string names;

    FSTRY

    if (!std::filesystem::is_directory(l_path)) {
        names += "-" + get_permissions(std::filesystem::status(l_path).permissions()) + std::to_string(std::filesystem::hard_link_count(l_path)) + " owner owner " + std::to_string(std::filesystem::file_size(l_path)) + format_file_time(std::filesystem::last_write_time(l_path)) + l_path;
        return names;
    }

    for (const auto& entry : std::filesystem::directory_iterator(l_path)) {

        if (entry.is_directory()) {
            names += "d" + get_permissions(entry.status().permissions()) + std::to_string(entry.hard_link_count()) + " owner group " + "4096" + format_file_time(entry.last_write_time()) + entry.path().filename().string();
        } else {
            names += "-" + get_permissions(entry.status().permissions()) + std::to_string(entry.hard_link_count()) + " owner group " + std::to_string(entry.file_size()) + format_file_time(entry.last_write_time()) + entry.path().filename().string();
        }
        names += "\r\n";
    }

    FSCATCH

    return names;
}

void retr(Client* client, const std::string& m_path, size_t m_offset) {

    if (client == nullptr) {
        LOG("from sendfile() : client is nullptr returning.");
        return;
    }

    std::ifstream input(m_path, std::ios::binary);

    if (input.is_open()) {

        input.seekg(m_offset, std::ios::beg);

        const size_t bufferSize = 4096;
        char buffer[bufferSize];

        while (!input.eof()) {
            input.read(buffer, bufferSize);
            size_t bytesReadThisRound = input.gcount();

            if (client->m_write(buffer, bytesReadThisRound) <= 0) {
                LOG("from sendfile() : failed to send file breaking the loop.");
                break;
            }
        }

        input.close();
    } else {
        LOG("Failed to open file from sendFile()");
    }
}

void stor(Client* client, const std::string& m_path, size_t m_offset) {

    std::ofstream output(m_path, std::ios::binary);

    if (output.is_open()) {

        output.seekp(m_offset, std::ios::beg);

        const size_t bufferSize = 4096;
        char buffer[bufferSize];
        int dataRead = 1;

        while (dataRead > 0) {
            dataRead = client->m_read(bufferSize, buffer);

            if (dataRead > 0) {
                output.write(buffer, dataRead);
            }
        }

        output.close();

    } else {
        LOG("failed to send the file error from recieveFile()");
    }
}

bool mkd(const std::string& l_path) {

    FSTRY

    if (!std::filesystem::exists(l_path)) {

        if (std::filesystem::create_directory(l_path)) {
            LOG("created a new directory " << l_path);
            return true;
        }
    }

    FSCATCH

    return false;
}

bool renameFile(const std::string& l_oldname, const std::string& l_newname) {

    FSTRY

    std::filesystem::rename(l_oldname, l_newname);

    return true;

    FSCATCH

    return false;
}
