#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <signal.h>
#include <sockets.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"

Client::Client(int socket) {
    this->id = socket;
}

int Client::getId() {
    return this->id;
}

bool Client::setTimeout(int milliseconds) {
    timeval timeout;
    timeout.tv_sec = milliseconds;
    timeout.tv_usec = 0;

    int result = setsockopt(this->id, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    return !(result < 0);
}

std::string Client::read(int& dataRead) {

    std::string buffer;
    char raw[4096];

    int readData = ::read(this->id, raw, sizeof(raw));

    dataRead = readData;
    raw[readData] = 0;
    buffer = raw;

    return buffer;
}

int Client::m_read(const int& bufferSize, char* o_buffer) {
    return ::read(this->id, o_buffer, bufferSize);
}

int Client::write(const char* data) {
    return ::write(this->id, data, strlen(data));
}

int Client::m_write(const char* data, const size_t& size) {
    signal(SIGPIPE, SIG_IGN);
    return ::write(this->id, data, size);
}

void Client::close() {
    ::close(this->id);
}

Client::~Client() {
    ::close(this->id);
    LOG("client destroyed");
}

// socket class function definitions ---------------------------

ServerSocket::ServerSocket(const int& port) {

    this->port = port;
    this->result = true;
    this->server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server < 0) {
        this->result = false;
        throw std::runtime_error("failed to create server");
    }

    int opt = 1;
    setsockopt(this->server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        close(this->server);
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

    if (server < 0) {
        this->result = false;
        throw std::runtime_error("failed to create server");
    }

    int opt = 1;
    setsockopt(this->server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    std::set<int> ports;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(start_port, end_port);

    while (ports.size() < (end_port - start_port)) {

        this->port = dist(gen);
        serverAddress.sin_port = htons(this->port);

        if (bind(server, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            ports.insert(this->port);
        } else {
            return;
        }
    }

    close(this->server);
    this->result = false;
    throw std::runtime_error("failed to bind to the socket with ports with range from " + std::to_string(start_port) + " to " + std::to_string(end_port));
}

void ServerSocket::start() {

    if (listen(server, 10) < 0) {
        close(this->server);
        this->result = false;
        throw std::runtime_error("failed to listen to the socket");
    }

    LOG("Server is listening on port " << this->port);
}

std::unique_ptr<Client> ServerSocket::getClient() {

    if (result) {

        int clientSocket = accept(this->server, 0, 0);

        if (!(clientSocket < 0)) {
            return std::make_unique<Client>(clientSocket);
        } else {
            throw std::runtime_error("function accept returned with -1 !");
        }

    } else {
        ::close(this->server);
        throw std::runtime_error("can't get client, socket is not in listening state!");
    }

    return nullptr;
}

int ServerSocket::getPort() {

    return this->port;
}

int ServerSocket::waitTill(const int& sec) {

    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;

    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(this->server, &fd);

    int selectResult = select(this->server + 1, &fd, nullptr, nullptr, &timeout);

    if (selectResult < 0) {
        ::close(this->server);
        throw std::runtime_error("failed to wait for client with select, error occoured!");
    } else if (selectResult == 0) {
        return 1;
    }

    return 0;
}

ServerSocket::~ServerSocket() {
    ::shutdown(this->server, SHUT_RDWR);
    ::close(this->server);
    LOG("server with port " << this->port << " destroyed");
}

std::string getIpAddress() {

    struct ifaddrs* ifAddrStruct = NULL;
    struct ifaddrs* ifa = NULL;
    void* tmpAddrPtr = NULL;
    std::string ipAddress;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {

        if (ifa->ifa_addr->sa_family == AF_INET && (ifa->ifa_flags & IFF_UP)) {

            tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            std::string ipBuffer = ifa->ifa_name;

            if (ipBuffer != "lo") {
                ipAddress = addressBuffer;
            }
        }
    }

    for (int i = 0; i < sizeof(ipAddress); i++) {
        if (ipAddress[i] == '.') {
            ipAddress[i] = ',';
        }
    }

    if (ifAddrStruct != NULL) {
        freeifaddrs(ifAddrStruct);
    }

    return ipAddress;
}
