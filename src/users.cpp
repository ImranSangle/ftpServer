#include <cstdlib>
#include <fstream>
#include <stdexcept>

#include "json.hpp"
#include "log.h"
#include "users.h"

using namespace nlohmann;

static json j_users;

bool load_user_data() {

    const char* env = std::getenv("HOME");

    if (env != nullptr) {

        std::ifstream input(std::string(env) + "/.config/iftp/iftp_users.json", std::ios::binary);

        if (input.is_open()) {
            j_users = json::parse(input);
            input.close();
        } else {
            LOG("Error: Failed to open users data file!");
            return false;
        }

    } else {
        std::runtime_error("Error: HOME Environment variable not set!");
    }

    return true;
}

void authenticate(User& current_session) {

    if (j_users[current_session.username]["password"] == current_session.password) {

        current_session.is_authenticated = true;
        current_session.path.setPath(std::string(j_users[current_session.username]["path"]).c_str());
        current_session.writable = j_users[current_session.username]["writable"];
    }
};
