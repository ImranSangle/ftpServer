#pragma once

#include <string>

#include "stringFunctions.h"

struct User {
    std::string username;
    std::string password;
    Browze path = {" ","/"};
    bool writable = true;
    bool is_authenticated = false;
};

bool authenticate(User& current_session);
