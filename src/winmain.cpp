#include <cstdlib>
#include <iostream>
#include <minwindef.h>
#include <string>
#include <synchapi.h>
#include <thread>
#include <windows.h>

#include <stringFunctions.h>
#include <winsock.h>

static bool sendD = false;
static int port = 3001;

//status codes--------------------------------------------
static const char* ready = "220 service ready for new user\r\n";
static const char* askPassword = "331 username okay, password for username\r\n";
static const char* loggedIn  = "230 logged in, proceed\r\n";
static const char* ok  = "200 command type okay\r\n";
static const char* syntaxError  = "500 syntax error\r\n";
static const char* systemType  = "215 UNIX Type: Apache FtpServer\r\n";
static const char* passiveMode  = "227 entering passive mode\n(192,168,31,2,11,185)\r\n";
static const char* directoryChanged  = "250 directory changed to /\r\n";
static const char* currentworkingdirctory = "257 '/' is current directory\r\n";
static const char* ePassiveMode = "229 Entering Passive Mode (|||";
static const char* fileStatusOkay = "150 File status okay; about to open data connection\r\n";
static const char* closingDataConnection = "226 Closing data connection\r\n";
static const char* abortSuccesfull = "226 ABOR command succesfull\r\n";
static const char* notImplemented = "502 command not implemented\r\n";

class Client{
  SOCKET id;
  public:
  Client(SOCKET socket){
    this->id = socket;
  }

  bool isConnected(){
    struct sockaddr_in addr;
    int length = sizeof(addr);
    int result = getpeername(this->id,(struct sockaddr*)&addr,&length);
   // std::cout<<"the result is "<<result<<std::endl;
    return (result != -1);
  }
  
  std::string read(int* dataRead){
    std::string buffer;
    char raw[1000];
    char command[100];
    int readData = ::recv(this->id,raw,sizeof(raw),0);
    *dataRead = readData;
    raw[readData] = 0;
    getWordAt(raw,command,0);
    buffer = command;
    return buffer;
  }

  int write(const char* data){
    return  ::send(this->id,data,strlen(data),0);
  }

  void close(){
    ::closesocket(this->id);
    delete this;
  }
  
  ~Client(){
    std::cout<<"client destroyed"<<std::endl;
  }
};

class ServerSocket{
  SOCKET server;
  int port;
  sockaddr_in serverAddress;
  bool socketCreated = false;
  bool socketBound = false;
  bool socketListening = false;

  public:
  ServerSocket(int port){
    this->port = port;
    this->server = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(server < 0){
      std::cout<<"failed to create server"<<std::endl;
      std::exit(-1);
    }
    socketCreated = true;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;

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
    std::cout<<"Server is listening on port "<<port<<std::endl;
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
    closesocket(this->server);
    std::cout<<"server destroyed"<<std::endl;
  }

};

void startPassiveMode(int port){

  ServerSocket socket(port);

  socket.start();

  Client* client = socket.getClient();

  std::cout<<"someone connected to passive server on port : "<<port<<std::endl;

  while(true){
    if(sendD){
      client->write("Android\r\nDownloads\r\nMusic\r\nMovies\r\nothers\r\nVideos\r\n");
      std::cout<<"file list sent succesfully.."<<std::endl;
      sendD = false;
      client->close(); 
      return;
    }
    Sleep(200);
  }

}

void loadwsa(){
  WSADATA ws;
  if(WSAStartup(MAKEWORD(2,2),&ws) != 0){
    std::cout<<"failed to load the dll closing..."<<std::endl;
    exit(1);
  }
}

void serviceWorker(Client* client){

    client->write(ready);

    while(client->isConnected()){
      int readData;
      std::string command = client->read(&readData);
      if(readData <= 0){
        std::cout<<"exiting from small loop"<<std::endl;
        break;
      }
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
      if(command  == "CWD"){
        std::cout<<"client is asking for changing directory changing to /"<<std::endl;
        client->write(directoryChanged);
      }
      if(command == "PWD\r"){
        std::cout<<"client is asking the current working directory sending"<<std::endl; 
        client->write(currentworkingdirctory);
      }
      if(command  == "PASV\r"){
        std::cout<<"starting passive mode"<<std::endl;
        std::thread worker(startPassiveMode,port);
        worker.detach();
        client->write(passiveMode);
        port++;
      }
      if(command == "EPSV" || command == "EPSV\r"){
        std::cout<<"starting epsv mode"<<std::endl;
        std::thread worker(startPassiveMode,port);
        worker.detach();
        client->write((std::string(ePassiveMode)+std::to_string(port)+std::string("|)\r\n")).c_str());
        port++;
      }
      if(command == "NLST" || command == "NLST\r"){
        std::cout<<"asked for nlst sending list"<<std::endl;
        client->write(fileStatusOkay);
        sendD = true;
        Sleep(200);
        client->write(closingDataConnection);
        std::cout<<"passive data connection closed"<<std::endl;
      }
      if(command == "ABOR" || command == "ABOR\r"){
        std::cout<<"asking for abortion aborting.."<<std::endl;
        client->write(abortSuccesfull);
      }
      if(command == "MLSD" || command == "MLSD\r"){
        std::cout<<"asked MLSD sending not implemented"<<std::endl;
        client->write(notImplemented);
      }
      if(command == "LIST" || command == "LIST\r"){
        std::cout<<"asked LIST sending not implemented"<<std::endl;
        client->write(notImplemented);
      }
      if(command  == "QUIT\r"){
        std::cout<<"asked to quit the server quitting..."<<std::endl;
        break;
      }
    }

    client->close();

}

int main(){

  loadwsa();

  ServerSocket socket(3000);

  socket.start();

  while(true){
    std::cout<<"service ready waiting for new connections..."<<std::endl;  

    Client* client = socket.getClient();

    std::cout<<"new client connected sending service ready"<<std::endl;  

    std::thread worker(serviceWorker,client);

    worker.detach();

  }

  WSACleanup();

  return 0;
}
