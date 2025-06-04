#include <cstddef>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "log.h"
#include "sockets.h"

// client class function definitions ---------------------------
Client::Client(SOCKET socket) {
    this->id = socket;
}

SOCKET Client::getId() {
    return this->id;
}

bool Client::setTimeout(int milliseconds) {
    timeval timeout;
    timeout.tv_sec = milliseconds;
    timeout.tv_usec = 0;

    int result = setsockopt(this->id, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    return result != SOCKET_ERROR;
}

std::string Client::read(int& dataRead) {
    std::string buffer;
    char raw[4096];

    int readData = ::recv(this->id, raw, sizeof(raw), 0);
    dataRead = readData;
    raw[readData] = 0;

    buffer = raw;
    return buffer;
}

int Client::m_read(const int& bufferSize, char* o_buffer) {
    return ::recv(this->id, o_buffer, bufferSize, 0);
}

int Client::write(const char* data) {
    return ::send(this->id, data, strlen(data), 0);
}

int Client::m_write(const char* data, const size_t& size) {
    return ::send(this->id, data, size, 0);
}

void Client::close() {
    ::closesocket(this->id);
}

Client::~Client() {
    ::closesocket(this->id);
    LOG("client destroyed");
}

// socket class function definitions ---------------------------

ServerSocket::ServerSocket(const int& port) {

    this->port = port;
    this->result = true;
    this->server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server == INVALID_SOCKET) {
        this->result = false;
        throw std::runtime_error("failed to create server");
    }

    int opt = 1;
    setsockopt(this->server, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        closesocket(this->server);
        this->result = false;
        throw std::runtime_error("failed to bind to the socket with port " + std::to_string(this->port));
    }
}

ServerSocket::ServerSocket(const int& start_port, const int& end_port) {

    if (start_port > end_port) {
        throw std::logic_error("start_port is larger than end_port!");
    }

    this->result = true;
    this->server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server == INVALID_SOCKET) {
        this->result = false;
        throw std::runtime_error("failed to create server");
    }

    int opt = 1;
    setsockopt(this->server, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;

    std::set<int> ports;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(start_port, end_port);

    while (ports.size() < (end_port - start_port)) {

        this->port = dist(gen);
        serverAddress.sin_port = htons(this->port);

        if (bind(server, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
            ports.insert(this->port);
        } else {
            return;
        }
    }

    closesocket(this->server);
    this->result = false;
    throw std::runtime_error("failed to bind to the socket with port " + std::to_string(this->port));
}

void ServerSocket::start() {

    if (listen(server, 10) == SOCKET_ERROR) {
        closesocket(this->server);
        this->result = false;
        throw std::runtime_error("failed to listen to the socket");
    }

    LOG("Server is listening on port " << this->port);
}

std::unique_ptr<Client> ServerSocket::getClient() {

    if (result) {

        SOCKET clientSocket = accept(this->server, 0, 0);

        if (clientSocket != INVALID_SOCKET) {
            return std::make_unique<Client>(clientSocket);
        } else {
            throw std::runtime_error("function accept returned with -1 !");
        }

    } else {
        closesocket(this->server);
        throw std::runtime_error("can't get client, socket is not in listening state!");
    }

    return nullptr;
}

int ServerSocket::getPort() {

    return this->port;
}

int ServerSocket::waitTill(const int& sec) {

    timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;

    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(this->server, &fd);

    int selectResult = select(0, &fd, nullptr, nullptr, &timeout);

    if (selectResult == SOCKET_ERROR) {
        ::closesocket(this->server);
        throw std::runtime_error("failed to wait for client with select, error occoured!");
    } else if (selectResult == 0) {
        return 1;
    }

    return 0;
}

ServerSocket::~ServerSocket() {
    ::shutdown(this->server, SD_BOTH);
    ::closesocket(this->server);
    LOG("server with port " << this->port << " destroyed");
}

std::string getIpAddress() {

    char hostname[256];

    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        LOG("from getIpAddress() : getting hostname failed.");
        return NULL;
    }

    struct sockaddr_in sockaddr_ipv4;
    struct hostent* pHost;

    pHost = gethostbyname(hostname);

    if (pHost == NULL) {
        LOG("from getIpAddress() : gethostbyname failed.");
        return NULL;
    }

    sockaddr_ipv4.sin_family = AF_INET;
    sockaddr_ipv4.sin_addr.S_un.S_addr = *((unsigned long*)pHost->h_addr);

    char ipAddr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sockaddr_ipv4.sin_addr, ipAddr, INET_ADDRSTRLEN);

    for (int i = 0; i < sizeof(ipAddr); i++) {
        if (ipAddr[i] == '.') {
            ipAddr[i] = ',';
        }
    }

    return ipAddr;
}
