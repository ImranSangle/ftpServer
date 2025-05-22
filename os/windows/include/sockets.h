#pragma once

#include <_timeval.h>
#include <winsock2.h>
#include <string>

class Client{

    SOCKET id;
    public:

    Client(SOCKET socket);
    
    SOCKET getId();

    bool setTimeout(int);
    
    std::string read(int& dataRead);

    int m_read(const int&,char*);

    int write(const char* data);

    int m_write(const char*,const size_t&);

    void close();
    
    ~Client();
};


class ServerSocket{

    SOCKET server;
    int port;
    sockaddr_in serverAddress;
    bool result;

    public:
    ServerSocket(const int& port);

    ServerSocket(const int& start_port,const int& end_port);
    
    void start();

    Client* getClient();

    int getPort();

    int waitTill(const int&);

    ~ServerSocket();

};

std::string getIpAddress();
