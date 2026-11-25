#include <cstdlib>
#include <fstream>
#include <sodium.h>
#include <stdexcept>

#include "json.hpp"
#include "log.h"
#include "users.h"

using namespace nlohmann;

static json j_users;

static std::string hash_password(const std::string& password) {

    if (sodium_init() < 0) {
        throw std::runtime_error("libsodium initialization failed");
    }

    char hash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(hash, password.c_str(), password.size(), crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("password hashing failed");
    }

    return std::string(hash);
}

static bool verify_password(const std::string& stored_hash, const std::string& password) {

    return crypto_pwhash_str_verify(stored_hash.c_str(), password.c_str(), password.size()) == 0;
}

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
        throw std::runtime_error("Error: HOME Environment variable not set!");
    }

    return true;
}

void authenticate(User& current_session) {

    if (verify_password(j_users[current_session.username]["password"], current_session.password)) {

        current_session.is_authenticated = true;
        current_session.path.setDrive(std::string(j_users[current_session.username]["drive"]).c_str());
        current_session.path.setPrefixPath(std::string(j_users[current_session.username]["path"]).c_str());
        current_session.writable = j_users[current_session.username]["writable"];
    }
};
