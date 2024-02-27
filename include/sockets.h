#pragma once

#include <_timeval.h>
#include <cstddef>
#ifdef _WIN64

#include <winsock2.h>
#include <string>

class Client{
  SOCKET id;
  public:
  Client(SOCKET socket);

  bool isConnected();

  bool setTimeout(int seconds);
  
  std::string read(int& dataRead);

  int write(const char* data);

  int m_write(const char*,size_t);

  void close();
  
  ~Client();
};


class ServerSocket{
  SOCKET server;
  int port;
  sockaddr_in serverAddress;
  bool socketCreated = false;
  bool socketBound = false;
  bool socketListening = false;

  public:
  ServerSocket(int port);
  
  void start();

  Client* getClient();

  int waitTill(int);

  ~ServerSocket();

};
#endif 
