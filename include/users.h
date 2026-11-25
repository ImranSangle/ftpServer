#pragma once

#include <string>

#include "macros.h"
#include "stringFunctions.h"

struct User {
    std::string username;
    std::string password;
    Browze path = DEFAULT_PATH;
    bool writable = false;
    bool is_authenticated = false;
    bool failed_attempt = false;
};

bool load_user_data();

void authenticate(User& current_session);
