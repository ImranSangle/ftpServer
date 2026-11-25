#pragma once

#ifdef WIN64

#define SYSTEM_TYPE systemTypeWindows

#define DEFAULT_PATH {"C:","/"}

#endif // WIN64

#ifdef __linux__

#define SYSTEM_TYPE systemTypeLinux

#define DEFAULT_PATH {"","/"}

#endif // __linux__

#define FSDITRY try {

#define FSDICATCH                                                                                      \
    }                                                                                                  \
    catch (const std::filesystem::filesystem_error& ex) {                                              \
        std::cerr << "File system error: " << ex.what() << std::endl;                                  \
        std::cerr << "Path involved: " << ex.path1().string() << std::endl;                            \
        std::cerr << "Error code: " << ex.code().value() << " - " << ex.code().message() << std::endl; \
        continue;                                                                                      \
    }                                                                                                  \
    catch (const std::exception& ex) {                                                                 \
        std::cerr << "Exception: " << ex.what() << std::endl;                                          \
        continue;                                                                                      \
    }

#define FSTRY try {

#define FSCATCH                                                                                        \
    }                                                                                                  \
    catch (const std::filesystem::filesystem_error& ex) {                                              \
        std::cerr << "File system error: " << ex.what() << std::endl;                                  \
        std::cerr << "Path involved: " << ex.path1().string() << std::endl;                            \
        std::cerr << "Error code: " << ex.code().value() << " - " << ex.code().message() << std::endl; \
    }                                                                                                  \
    catch (const std::exception& ex) {                                                                 \
        std::cerr << "Exception: " << ex.what() << std::endl;                                          \
    }

#define AUTH_CHECK                               \
    if (!current_session.is_authenticated) {     \
        client->write("530 Access denied!\r\n"); \
        continue;                                \
    }
