#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <string>

#include <stringFunctions.h>

static const char* ready = "220 service ready for new user\r\n";
static const char* askPassword = "331 username okay, password for username\r\n";
static const char* loggedIn  = "230 logged in, proceed\r\n";
static const char* ok  = "200 command type okay\r\n";
static const char* syntaxError  = "500 syntax error\r\n";
static const char* systemType  = "215 UNIX Type: Apache FtpServer\r\n";

class Client{
  int id;
  public:
  Client(int socket){
    this->id = socket;
  }
  std::string read(){
    std::string buffer;
    char raw[1000];
    char command[100];
    int readData = ::read(this->id,raw,sizeof(raw));
    raw[readData] = 0;
    getWordAt(raw,command,0);
    buffer = command;
    return buffer;
  }
  int write(const char* data){
    return  ::write(this->id,data,strlen(data));
  }
  void close(){
    ::close(this->id);
  }
  ~Client(){
    std::cout<<"client destroyed"<<std::endl;
  }
};

class ServerSocket{
  int server;
  sockaddr_in serverAddress;
  public:
  ServerSocket(int port){
    this->server = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    bind(server,(sockaddr*)&serverAddress,sizeof(serverAddress)); 
  }
  void start(){
    listen(server,10);
    std::cout<<"Server is listening on port "<<std::endl;
  }
  Client* getClient(){
    Client* client  = new Client(accept(this->server,0,0)); 
    return client;
  }
  ~ServerSocket(){
    close(this->server);
    std::cout<<"server destroyed"<<std::endl;
  }

};


int main(){

  ServerSocket socket(3000);

  socket.start();

  while(true){
    Client* client = socket.getClient();

    client->write(ready);

    if(client->read() == "USER"){
      std::cout<<"username is correct asking for password"<<std::endl;
      client->write(askPassword);
    }
    if(client->read() == "PASS"){
      std::cout<<"password is corrrect asking for method"<<std::endl;
      client->write(loggedIn);
    }
    if(client->read() == "TYPE"){
      std::cout<<"client want's type binary accepting"<<std::endl;
      client->write(ok);
    }
    if(client->read() == "FEAT"){
      std::cout<<"client is asking for features, sending not found"<<std::endl;
      client->write(syntaxError);
    }
    if(client->read() == "SYST"){
      std::cout<<"client is asking for system, sending linux system"<<std::endl;
      client->write(systemType);
    }
    //std::cout<<client->read()<<std::endl;

    client->close();

    delete client;
  }


  return 0;
}
