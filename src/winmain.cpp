#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <minwindef.h>
#include <string>
#include <algorithm>
#include <synchapi.h>
#include <thread>
#include <winsock2.h>
#include <filesystem>
#include <fstream>

#include "sockets.h"
#include "stringFunctions.h"


static int sendD = 0;
static int port = 3001;
std::string globalPath;

//status codes--------------------------------------------
static const char* ready = "220 service ready for new user\r\n";
static const char* askPassword = "331 username okay, password for username\r\n";
static const char* loggedIn  = "230 logged in, proceed\r\n";
static const char* ok  = "200 command type okay\r\n";
static const char* syntaxError  = "500 syntax error\r\n";
static const char* systemType  = "215 windows_NT\r\n";
static const char* passiveMode  = "227 entering passive mode (192,168,31,2,";
static const char* directoryChanged  = "250 directory changed to ";
static const char* ePassiveMode = "229 Entering Passive Mode (|||";
static const char* fileStatusOkay = "150 File status okay; about to open data connection\r\n";
static const char* closingDataConnection = "226 Closing data connection\r\n";
static const char* abortSuccesfull = "226 ABOR command succesfull\r\n";
static const char* notImplemented = "502 command not implemented\r\n";
static const char* transferComplete = "226 transfer complete\r\n";

std::string nlst(const char* path){
  std::string names;
  for(const auto &entry : std::filesystem::directory_iterator(path)){
    names+=entry.path().filename().string();
    names+="\r\n";
  }
   return names;
}

std::string list(const char* path){
  std::string names;
  for(const auto &entry : std::filesystem::directory_iterator(path)){
      if(entry.is_directory()){
      names+= "drwxrwxrwx 2 sharique sharique 4096 Feb 25 00:54 "+entry.path().filename().string();
      }else{
      names+= "-rwxrwxrwx 1 sharique sharique "+std::to_string(entry.file_size())+" Feb 25 00:54 "+entry.path().filename().string();
      }
      names+= "\r\n";
    }
   return names;
}

void sendFile(Client* client){
  std::ifstream input(globalPath);

  if(input.is_open()){
    input.seekg(0,std::ios::end);
    size_t fileSize = input.tellg();
    input.seekg(0,std::ios::beg);

    char* buffer = new char[fileSize];
    input.read(buffer,fileSize);
    buffer[fileSize] = 0;
    client->m_write(buffer,fileSize);
    delete[] buffer;
  }else{
    std::cout<<"failed to open file from sendfile()"<<std::endl;
  }
}

std::string removeExtras(const std::string& value){
  std::string m_fileName;
  size_t findResult = value.find("\r");

  if(findResult == std::string::npos){
    findResult = value.find("\n");
    if(findResult == std::string::npos){
       m_fileName = value.substr(0,value.length());
    }else{
       m_fileName = value.substr(0,findResult);
    }
  }else{
       m_fileName = value.substr(0,findResult);
  }
  return m_fileName;
}

int getSize(const std::string& path,const std::string& filename){
  return std::filesystem::file_size(path+"/"+removeExtras(filename));
}

void startPassiveMode(int port){

  ServerSocket socket(port);

  socket.start();

  if(socket.waitTill(10)){
    std::cout<<"timeout! closing the passive server with port : "<<port<<std::endl;
    return;
  }

  Client* client = socket.getClient();

  client->setTimeout(100);

  std::cout<<"someone connected to passive server on port : "<<port<<std::endl;

  while(true){
    if(sendD == 1){
      client->write(list(globalPath.c_str()).c_str());
      std::cout<<"list sent succesfully.."<<std::endl;
      sendD = 0;
      client->close(); 
      return;
    }
    if(sendD == 2){
      sendFile(client);
      std::cout<<"file sent succesfully.."<<std::endl;
      sendD = 0;
      client->close();  
      return;
    }
    int readData;
    client->read(readData);
    if(readData == 0){
       std::cout<<"client disconnected the session on port : " <<port<<" closing this connection"<<std::endl;
      client->close();
      break;
    }
  }

}

#ifdef _WIN64

void loadwsa(){
  WSADATA ws;
  if(WSAStartup(MAKEWORD(2,2),&ws) != 0){
    std::cout<<"failed to load the dll closing..."<<std::endl;
    exit(1);
  }
}

#endif

void serviceWorker(Client* client){

    client->write(ready);

    Browze path("G:","/");

    while(true){
      int readData;
      std::string fullCommand = client->read(readData);
      std::string command = getCode(fullCommand.c_str());
      std::string subCommand = getCommand(fullCommand.c_str());

      std::transform(command.begin(), command.end(),command.begin(), [](unsigned char c){ return std::toupper(c);});

      if(readData <= 0){
        std::cout<<"client's main connection disconnected closing serviceWorker"<<std::endl;
        break;
      }
      std::cout<<fullCommand<<std::endl;

      if(command  == "USER" || command == "USER\r"){
        std::cout<<"username is correct asking for password"<<std::endl;
        client->write(askPassword);
      }
      if(command  == "PASS" || command == "PASS\r"){
        std::cout<<"password is corrrect asking for method"<<std::endl;
        client->write(loggedIn);
      }
      if(command == "TYPE" || command  == "TYPE\r"){
        std::cout<<"client want's type binary accepting"<<std::endl;
        client->write(ok);
      }
      if(command == "SIZE" || command == "SIZE\r"){
        std::cout<<"client is asking for size sending details"<<std::endl;
        int size = getSize(path.getPath(),subCommand);
        client->write((std::string("213 ")+std::to_string(size)+"\r\n").c_str());
      }
      if(command == "FEAT" || command  == "FEAT\r"){
        std::cout<<"client is asking for features, sending not found"<<std::endl;
        client->write(syntaxError);
      }
      if(command == "SYST" || command  == "SYST\r"){
        std::cout<<"client is asking for system, sending linux system"<<std::endl;
        client->write(systemType);
      }
      if(command == "CWD" || command  == "CWD\r"){
        std::cout<<"client is asking for changing directory changing to /"<<std::endl;
        path.to(removeExtras(subCommand).c_str());
        client->write((directoryChanged+path.getPath()+"\r\n").c_str());
      }
      if(command == "CDUP" || command == "CDUP\r"){
        path.up();
        client->write((directoryChanged+path.getPath()+"\r\n").c_str());
      }
      if(command == "PWD"  || command == "PWD\r"){
        std::cout<<"client is asking the current working directory sending"<<std::endl; 
        client->write(("257 \""+path.getPath()+"\"is the current directory\r\n").c_str());
      }
      if(command == "PASV" || command  == "PASV\r"){
        std::cout<<"starting passive mode"<<std::endl;
        std::thread worker(startPassiveMode,port);
        worker.detach(); 
        int p1 = port/256;
        int p2 = port%256;
        client->write((std::string(passiveMode)+std::to_string(p1)+","+std::to_string(p2)+")\r\n").c_str());
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
        sendD = 1;
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
        std::cout<<"asked LIST sending file list.."<<std::endl;
        client->write(fileStatusOkay);
        if(subCommand.length() > 9){
        std::string newFilePath = "/"+subCommand.substr(subCommand.find(" ")+1,subCommand.length());
        newFilePath = removeExtras(newFilePath);
        path.setPath(newFilePath.c_str());
        }else{
        path.setPath("/");
        }
        globalPath = path.getPath();
        sendD = 1;
        Sleep(200);
        client->write(closingDataConnection);
        std::cout<<"passive data connection closed"<<std::endl;
      }
      if(command == "RETR" || command == "RETR\r"){
        std::cout<<"asking for a file sending file to the client"<<std::endl;
        client->write(fileStatusOkay);
        globalPath = path.getDrive()+"/"+removeExtras(subCommand);
        sendD = 2;
        Sleep(500);
        client->write(transferComplete);
        std::cout<<"passive data connection closed"<<std::endl;
      }
      if(command == "OPTS" || command == "OPTS\r"){
        std::cout<<"asking for options sending not implemented"<<std::endl;
        client->write(ok);
      }
      if(command == "QUIT" || command  == "QUIT\r"){
        std::cout<<"asked to quit the server quitting..."<<std::endl;
        client->write("goodbye");
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
    std::cout<<"main thread ready ready waiting for new connections..."<<std::endl;  

    Client* client = socket.getClient();

    std::cout<<"new client connected starting serviceWorker & sending service ready"<<std::endl;  

    std::thread worker(serviceWorker,client);

    worker.detach();

  }

  #ifdef _WIN64
  WSACleanup();
  #endif

  return 0;
}
