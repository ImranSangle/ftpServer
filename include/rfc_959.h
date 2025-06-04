#pragma once

#include <memory>
#include <string>

#include "sockets.h"

std::string nlst(const std::string& l_path);

std::string list(const std::string& l_path);

void retr(const std::unique_ptr<Client>& client, const std::string& m_path, size_t m_offset);

void stor(const std::unique_ptr<Client>& client, const std::string& m_path, size_t m_offset);

bool mkd(const std::string& l_path);

bool renameFile(const std::string& l_oldname, const std::string& l_newname);
