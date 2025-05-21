#include <cstddef>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "sockets.h"
#include "log.h"

//client class function definitions ---------------------------
Client::Client(SOCKET socket){
  this->id = socket;
}

SOCKET Client::getId(){
  return this->id; 
}

bool Client::setTimeout(int milliseconds){
  timeval timeout;
  timeout.tv_sec = milliseconds;
  timeout.tv_usec = 0;

  int result = setsockopt(this->id,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));

  return result != SOCKET_ERROR;
}

std::string Client::read(int& dataRead){
  std::string buffer;
  char raw[4096];
  //char command[100];
  int readData = ::recv(this->id,raw,sizeof(raw),0);
  dataRead = readData;
  raw[readData] = 0;
  //getWordAt(raw,command,0);
  buffer = raw;
  return buffer;
}

int Client::m_read(const int& bufferSize,char* o_buffer){
  
  return ::recv(this->id,o_buffer,bufferSize,0);
  
}

int Client::write(const char* data){
  return  ::send(this->id,data,strlen(data),0);
}

int Client::m_write(const char* data,const size_t& size){
  return ::send(this->id,data,size,0);
}

void Client::close(){
  ::closesocket(this->id);
  delete this;
}

Client::~Client(){
     LOG("client destroyed");
}

//socket class function definitions ---------------------------

ServerSocket::ServerSocket(const int& port){
    this->port = port;
    this->server = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(server == INVALID_SOCKET){
       LOG("failed to create server");
      std::exit(-1);
    }
    socketCreated = true;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;

    if(bind(server,(sockaddr*)&serverAddress,sizeof(serverAddress)) == SOCKET_ERROR){
       LOG("failed to bind the address to the socket with port "<<this->port);
      closesocket(this->server);
      std::exit(-1);
    }
    socketBound = true;
  }
  
  void ServerSocket::start(){
    if(socketCreated && socketBound){
    listen(server,10);
    socketListening = true;
       LOG("Server is listening on port "<<port);
    }else{
       LOG("socket is not created or bounded error starting the server");
    closesocket(this->server);
    std::exit(-1);
    }
  }

  Client* ServerSocket::getClient(){
    if(socketListening){
    SOCKET clientSocket = accept(this->server,0,0);
      if(clientSocket != INVALID_SOCKET){
        Client* client  = new Client(clientSocket); 
        return client;
      }else{
         LOG("from sockets.cpp = client socket is INVALID_SOCKET returning nullptr");
        return nullptr;
      }
    }else{
       LOG("error: cant get client socket is not in listening state");
    closesocket(this->server);
    std::exit(-1);
    //return nullptr;
    }
  }

  int ServerSocket::waitTill(const int& sec){

  timeval timeout;
  timeout.tv_sec = sec;
  timeout.tv_usec = 0;

  fd_set fd;
  FD_ZERO(&fd);
  FD_SET(this->server,&fd);

  int selectResult = select(0,&fd,nullptr,nullptr,&timeout);
  if(selectResult == SOCKET_ERROR){
       LOG("wait failed closing..");
     ::closesocket(this->server);
     std::exit(1);
  }else if(selectResult == 0){
    return 1;
  }

  return 0;

}  

ServerSocket::~ServerSocket(){
  ::closesocket(this->server);
   LOG("server with port "<<this->port<<" destroyed");
}

std::string getIpAddress(){
  
  char hostname[256];
  if(gethostname(hostname,sizeof(hostname)) == SOCKET_ERROR){
      LOG("from getIpAddress() : getting hostname failed.");
      return NULL;
  }

  struct sockaddr_in sockaddr_ipv4;
  struct hostent* pHost;
   
  pHost = gethostbyname(hostname);
  if(pHost == NULL){
    LOG("from getIpAddress() : gethostbyname failed.");
    return NULL;
  }

  sockaddr_ipv4.sin_family = AF_INET;
  sockaddr_ipv4.sin_addr.S_un.S_addr = *((unsigned long*)pHost->h_addr);

  char ipAddr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sockaddr_ipv4.sin_addr,ipAddr, INET_ADDRSTRLEN);

  for(int i=0;i<sizeof(ipAddr);i++){
     if(ipAddr[i] == '.'){
       ipAddr[i] = ',';
    }
  }

  return ipAddr;
}
