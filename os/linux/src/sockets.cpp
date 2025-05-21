#include <cstddef>
#include <sockets.h>
#include <iostream>
#include <signal.h>
#include <cstring> 

#include "log.h"

Client::Client(int socket){
  this->id = socket;
}

int Client::getId(){
  return this->id; 
}

bool Client::setTimeout(int milliseconds){
  timeval timeout;
  timeout.tv_sec = milliseconds;
  timeout.tv_usec = 0;

  int result = setsockopt(this->id,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));

  return !(result < 0);
}

std::string Client::read(int& dataRead){
  std::string buffer;
  char raw[4096];
  //char command[100];
  int readData = ::read(this->id,raw,sizeof(raw));
  dataRead = readData;
  raw[readData] = 0;
  //getWordAt(raw,command,0);
  buffer = raw;
  return buffer;
}

int Client::m_read(const int& bufferSize,char* o_buffer){
  
  return ::read(this->id,o_buffer,bufferSize);
  
}

int Client::write(const char* data){
  return  ::write(this->id,data,strlen(data));
}

int Client::m_write(const char* data,const size_t& size){
  signal(SIGPIPE,SIG_IGN);
  return ::write(this->id,data,size);
}

void Client::close(){
  ::close(this->id);
  delete this;
}

Client::~Client(){
     LOG("client destroyed");
}

//socket class function definitions ---------------------------

ServerSocket::ServerSocket(const int& port){
    this->port = port;
    this->server = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(server < 0){
       LOG("failed to create server");
      std::exit(-1);
    }
    socketCreated = true;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if(bind(server,(sockaddr*)&serverAddress,sizeof(serverAddress)) < 0){
       LOG("failed to bind the address to the socket with port "<<this->port);
      close(this->server);
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
    close(this->server);
    std::exit(-1);
    }
  }

  Client* ServerSocket::getClient(){
    if(socketListening){
    int clientSocket = accept(this->server,0,0);
      if(!(clientSocket < 0)){
        Client* client  = new Client(clientSocket); 
        return client;
      }else{
         LOG("from sockets.cpp = client socket is INVALID_SOCKET returning nullptr");
        return nullptr;
      }
    }else{
       LOG("error: cant get client socket is not in listening state");
    close(this->server);
    std::exit(-1);
    //return nullptr;
    }
  }

  int ServerSocket::waitTill(const int& sec){

  struct timeval timeout;
  timeout.tv_sec = sec;
  timeout.tv_usec = 0;

  fd_set fd;
  FD_ZERO(&fd);
  FD_SET(this->server,&fd);

  int selectResult = select(this->server+1,&fd,nullptr,nullptr,&timeout);
  if(selectResult < 0){
       LOG("wait failed closing..");
     ::close(this->server);
     std::exit(1);
  }else if(selectResult == 0){
    return 1;
  }

  return 0;

}  

ServerSocket::~ServerSocket(){
  ::close(this->server);
   LOG("server with port "<<this->port<<" destroyed");
}

std::string getIpAddress(){

  struct ifaddrs* ifAddrStruct = NULL;
  struct ifaddrs* ifa = NULL;
  void *tmpAddrPtr = NULL;
  std::string ipAddress;
  
  getifaddrs(&ifAddrStruct);

  for(ifa = ifAddrStruct;ifa != NULL ; ifa = ifa->ifa_next){
       if(ifa->ifa_addr->sa_family == AF_INET && (ifa->ifa_flags & IFF_UP)){
          
         tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
         char addressBuffer[INET_ADDRSTRLEN];
         inet_ntop(AF_INET,tmpAddrPtr,addressBuffer,INET_ADDRSTRLEN);

         std::string ipBuffer = ifa->ifa_name;
         if(ipBuffer != "lo"){
            ipAddress = addressBuffer;
         }

       }
  }

  for(int i=0;i<sizeof(ipAddress);i++){
     if(ipAddress[i] == '.'){
       ipAddress[i] = ',';
    }
  }

  if(ifAddrStruct != NULL){
     freeifaddrs(ifAddrStruct);
  }

  return ipAddress;
}
