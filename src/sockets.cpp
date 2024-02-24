
#include <psdk_inc/_ip_types.h>
#ifdef _WIN64
#include <sockets.h>
#include <stringFunctions.h>
#include <iostream>

//client class function definitions ---------------------------
Client::Client(SOCKET socket){
  this->id = socket;
}

bool Client::isConnected(){
  sockaddr_in addr;
  int length = sizeof(addr);
  int result = getpeername(this->id,reinterpret_cast<sockaddr*>(&addr),&length);
  // std::cout<<"the result is "<<result<<std::endl;
  if(result == SOCKET_ERROR){
    return false;
  }else{
    return true;
  }
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
  char raw[1000];
  //char command[100];
  int readData = ::recv(this->id,raw,sizeof(raw),0);
  dataRead = readData;
  raw[readData] = 0;
  //getWordAt(raw,command,0);
  buffer = raw;
  return buffer;
}

int Client::write(const char* data){
  return  ::send(this->id,data,strlen(data),0);
}

void Client::close(){
  ::closesocket(this->id);
  delete this;
}

Client::~Client(){
  std::cout<<"client destroyed"<<std::endl;
}

//socket class function definitions ---------------------------

ServerSocket::ServerSocket(int port){
    this->port = port;
    this->server = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(server == INVALID_SOCKET){
      std::cout<<"failed to create server"<<std::endl;
      std::exit(-1);
    }
    socketCreated = true;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;

    if(bind(server,(sockaddr*)&serverAddress,sizeof(serverAddress)) == SOCKET_ERROR){
      std::cout<<"failed to bind to the address to the socket"<<std::endl;
      std::exit(-1);
    }
    socketBound = true;
  }
  
  void ServerSocket::start(){
    if(socketCreated && socketBound){
    listen(server,10);
    socketListening = true;
    std::cout<<"Server is listening on port "<<port<<std::endl;
    }else{
    std::cout<<"socket is not created or bounded error starting the server"<<std::endl;
    std::exit(-1);
    }
  }

  Client* ServerSocket::getClient(){
    if(socketListening){
    Client* client  = new Client(accept(this->server,0,0)); 
    return client;
    }else{
    std::cout<<"error: cant get client socket is not in listening state"<<std::endl;
    std::exit(-1);
    //return nullptr;
    }
  }

  int ServerSocket::waitTill(int sec){

  timeval timeout;
  timeout.tv_sec = sec;
  timeout.tv_usec = 0;

  fd_set fd;
  FD_ZERO(&fd);
  FD_SET(this->server,&fd);

  int selectResult = select(0,&fd,nullptr,nullptr,&timeout);
  if(selectResult == SOCKET_ERROR){
     std::cout<<"wait failed closing.."<<std::endl;
     ::closesocket(this->server);
     std::exit(1);
  }else if(selectResult == 0){
    return 1;
  }

  return 0;

}  

  ServerSocket::~ServerSocket(){
    ::closesocket(this->server);
    std::cout<<"server destroyed"<<std::endl;
  }

#endif 