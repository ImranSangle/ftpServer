#pragma once

#include <cstddef>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string>

class Client{
  int id;
  public:
  Client(int socket);
  
  int getId();

  bool setTimeout(int);
  
  std::string read(int& dataRead);

  int m_read(const int&,char*);

  int write(const char* data);

  int m_write(const char*,const size_t&);

  void close();
  
  ~Client();
};


class ServerSocket{
  int server;
  int port;
  sockaddr_in serverAddress;
  bool socketCreated = false;
  bool socketBound = false;
  bool socketListening = false;

  public:
  ServerSocket(const int& port);
  
  void start();

  Client* getClient();

  int waitTill(const int&);

  ~ServerSocket();

};

std::string getIpAddress();
