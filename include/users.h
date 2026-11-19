#pragma once

#include <string>

#include "stringFunctions.h"

struct User {
    std::string username;
    std::string password;
#ifdef WIN32
    Browze path = {"F:","/"};
#endif
#ifdef __linux__
    Browze path = {"","/"};
#endif
    bool writable = false;
    bool is_authenticated = false;
    bool failed_attempt = false;
};

bool load_user_data();

void authenticate(User& current_session);
