#pragma once

#include <cstddef>
#ifdef _WIN64
#include <_timeval.h>

#include <winsock2.h>
#include <string>

class Client{
  SOCKET id;
  public:
  Client(SOCKET socket);
  
  SOCKET getId();

  bool isConnected();

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
#endif 

#ifdef __linux__

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>

class Client{
  int id;
  public:
  Client(int socket);
  
  int getId();

  bool isConnected();

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
#endif
