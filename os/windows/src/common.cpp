#include "log.h"
#include <winsock2.h>

void load_wsa() {
    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
        LOG("failed to load the dll closing...");
        exit(1);
    }
}

void unload_wsa() {
    WSACleanup();
}
