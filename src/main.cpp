#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <thread>

#include <stringFunctions.h>

static const char* ready = "220 service ready for new user\r\n";
static const char* askPassword = "331 username okay, password for username\r\n";
static const char* loggedIn  = "230 logged in, proceed\r\n";
static const char* ok  = "200 command type okay\r\n";
static const char* syntaxError  = "500 syntax error\r\n";
static const char* systemType  = "215 UNIX Type: Apache FtpServer\r\n";
static const char* passiveMode  = "227 entering passive mode\r\n(192,168,31,4,12,18)\r\n";
static const char* directoryChanged  = "250 directory changed to /\r\n";

class Client{
  int id;
  public:
  Client(int socket){
    this->id = socket;
  }

  bool isConnected(){
    struct sockaddr_in addr;
    socklen_t length = sizeof(addr);
    int result = getpeername(this->id,(struct sockaddr*)&addr,&length);
   // std::cout<<"the result is "<<result<<std::endl;
    return (result != -1);
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
    delete this;
  }
  
  ~Client(){
    std::cout<<"client destroyed"<<std::endl;
  }
};

class ServerSocket{
  int server;
  sockaddr_in serverAddress;
  bool socketCreated = false;
  bool socketBound = false;
  bool socketListening = false;

  public:
  ServerSocket(int port){
    this->server = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(server < 0){
      std::cout<<"failed to create server"<<std::endl;
      std::exit(-1);
    }
    socketCreated = true;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if(bind(server,(sockaddr*)&serverAddress,sizeof(serverAddress))){
      std::cout<<"failed to bind to the address to the socket"<<std::endl;
      std::exit(-1);
    }
    socketBound = true;
  }
  
  void start(){
    if(socketCreated && socketBound){
    listen(server,10);
    socketListening = true;
    std::cout<<"Server is listening on port "<<std::endl;
    }else{
    std::cout<<"socket is not created or bounded error starting the server"<<std::endl;
    std::exit(-1);
    }
  }

  Client* getClient(){
    if(socketListening){
    Client* client  = new Client(accept(this->server,0,0)); 
    return client;
    }else{
    std::cout<<"error: cant get client socket is not in listening state"<<std::endl;
    std::exit(-1);
    //return nullptr;
    }
  }

  ~ServerSocket(){
    close(this->server);
    std::cout<<"server destroyed"<<std::endl;
  }

};

void startPassiveMode(){
  ServerSocket socket(3090);

  socket.start();

  Client* client = socket.getClient();

  std::cout<<"someone connected to passive server"<<std::endl;

  std::cout<<client->read()<<std::endl;
}

int main(){

  ServerSocket socket(3000);

  socket.start();

  while(true){
    Client* client = socket.getClient();

    client->write(ready);

    while(client->isConnected()){
      std::string command = client->read();
      std::cout<<command<<std::endl;
      //std::cout<<(int)command[4]<<std::endl;

      if(command  == "USER"){
	std::cout<<"username is correct asking for password"<<std::endl;
	client->write(askPassword);
      }
      if(command  == "PASS"){
	std::cout<<"password is corrrect asking for method"<<std::endl;
	client->write(loggedIn);
      }
      if(command  == "TYPE"){
	std::cout<<"client want's type binary accepting"<<std::endl;
	client->write(ok);
      }
      if(command  == "FEAT\r"){
	std::cout<<"client is asking for features, sending not found"<<std::endl;
	client->write(syntaxError);
      }
      if(command  == "SYST\r"){
	std::cout<<"client is asking for system, sending linux system"<<std::endl;
	client->write(systemType);
      }
      if(command  == "CWD\r"){
	std::cout<<"client is asking for changing directory changing to /"<<std::endl;
	client->write(directoryChanged);
      }
      if(command  == "PASV\r"){
	std::cout<<"starting passive mode"<<std::endl;
	std::thread worker(startPassiveMode);
	worker.detach();
	client->write(passiveMode);
      }
      if(command  == "QUIT\r"){
	std::cout<<"asked to quit the server quitting..."<<std::endl;
	break;
      }
    }
    //std::cout<<client->read()<<std::endl;

    client->close();

  }


  return 0;
}
