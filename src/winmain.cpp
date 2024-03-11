#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <minwindef.h>
#include <mutex>
#include <string>
#include <algorithm>
#include <synchapi.h>
#include <thread>
#include <winsock2.h>
#include <filesystem>
#include <fstream>

#include "sockets.h"
#include "stringFunctions.h"


static int port = 3001;

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
static const char* rnfr = "350 Requested file action pending further information\r\n";
static const char* rnto = "250 Requested file action okay, file renamed\r\n";
static const char* rn_error = "503 Can't find the file which has to be renamed.\r\n";

std::string formatPath(std::string m_path){
     std::string r_path;
     if(m_path[0] != '/'){
       r_path = "/";
       r_path += m_path;
      }else{
       r_path = m_path; 
      }
      if(m_path[m_path.length()-1] == '/' && m_path.length() > 1){
        r_path.pop_back();
      }
      return r_path;
}

std::string nlst(const char* path){
  std::string names;
  for(const auto &entry : std::filesystem::directory_iterator(path)){
    names+=entry.path().filename().string();
    names+="\r\n";
  }
   return names;
}


std::string mlsd(const char* path){
  std::string names;
    for(const auto &entry : std::filesystem::directory_iterator(path)){

    try{
          if(entry.is_directory()){
            names+= "Size=0;Modify=20240228145553.000;Type=dir; "+entry.path().filename().string();
          }else{
            names+= "Size="+std::to_string(entry.file_size())+";Modify=20240228145553.000;Type=file; "+entry.path().filename().string();
          }
            names+="\r\n";
      }
      catch (const std::filesystem::filesystem_error& ex) {
          // Handle the file system error
          std::cerr << "File system error: " << ex.what() << std::endl;
          std::cerr << "Path involved: " << ex.path1().string() << std::endl;
          std::cerr << "Error code: " << ex.code().value() << " - " << ex.code().message() << std::endl;
          // Continue with the next iteration
          continue;
      }
      catch (const std::exception& ex) {
          // Handle other exceptions
          std::cerr << "Exception: " << ex.what() << std::endl;
          // Continue with the next iteration
          continue;
      }
    }

  return names;
}

std::string list(const char* path) {
    std::string names;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        try {

            if (entry.is_directory()) {
                names += "drwxrwxrwx 2 sharique sharique 4096 Feb 25 00:54 " + entry.path().filename().string();
            }
            else {
                names += "-rwxrwxrwx 1 sharique sharique " + std::to_string(entry.file_size()) + " Feb 25 00:54 " + entry.path().filename().string();
            }
            names += "\r\n";
        }
        catch (const std::filesystem::filesystem_error& ex) {
            std::cerr << "File system error: " << ex.what() << std::endl;
            std::cerr << "Path involved: " << ex.path1().string() << std::endl;
            std::cerr << "Error code: " << ex.code().value() << " - " << ex.code().message() << std::endl;
            continue;
        }
        catch (const std::exception& ex) {
            // Handle other exceptions
            std::cerr << "Exception: " << ex.what() << std::endl;
            // Continue with the next iteration
            continue;
        }
    }

    return names;
}

void sendFile(Client* client,const std::string& m_path,size_t m_offset) {
    std::ifstream input(m_path, std::ios::binary); // Open file in binary mode

    if (input.is_open()) {
        // input.seekg(0, std::ios::end);
        // size_t fileSize = input.tellg();
        // input.seekg(0, std::ios::beg);

        input.seekg(m_offset,std::ios::beg);

        const size_t bufferSize = 4096; // Adjust buffer size as needed
        char buffer[bufferSize];

        size_t bytesRead = 0;
        while (!input.eof()) {
            input.read(buffer, bufferSize);
            size_t bytesReadThisRound = input.gcount(); 

            if (bytesReadThisRound > 0) {
                client->m_write(buffer, bytesReadThisRound);
                bytesRead += bytesReadThisRound;
            }
        }

        // if (bytesRead != fileSize) {
        //     std::cout << "Error: File size mismatch."<<std::endl;
        // }

        input.close(); // Close the file after reading
    } else {
        std::cout << "Failed to open file from sendFile()" << std::endl;
    }
}

void recieveFile(Client* client,const std::string& m_path,size_t m_offset){
    std::ofstream output(m_path, std::ios::binary);

    if(output.is_open()){
       
      output.seekp(m_offset,std::ios::beg);

      const size_t bufferSize = 4096;
      char buffer[bufferSize];
      int dataRead;
      
      while(dataRead > 0){
          dataRead = client->m_read(bufferSize,buffer);

          if(dataRead > 0){
           output.write(buffer,dataRead);
          }
      }

       output.close();

    }else{
       std::cout<<"failed to send the file error from recieveFile()"<<std::endl;
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

bool isAbsolutePath(const std::string& m_path){

  if(m_path.find("/") == std::string::npos){
      return false;
  }
  return true;
}

bool mkd(const std::string& name,const Browze path){
     if(isAbsolutePath(name)){
        std::string c_name = formatPath(name);
       if(!std::filesystem::exists(path.getDrive()+c_name)){
         if(std::filesystem::create_directory(path.getDrive()+c_name)){
              std::cout<<"created a new directory "<<name<<std::endl;
              return true;
         }

       }

    }else if(!std::filesystem::exists(path.getPath()+"/"+name)){
           if(std::filesystem::create_directory(path.getPath()+"/"+name)){
                std::cout<<"created a new directory "<<name<<std::endl;
                return true;
            }
    }   

     return false;
}

bool renameFile(const std::string& m_oldname,const std::string& m_newname,const Browze& path){
  bool isOldAbsoulute = isAbsolutePath(m_oldname);
  bool isNewAbsoulute = isAbsolutePath(m_newname);
  std::string n_oldname = formatPath(m_oldname);
  std::string n_newname = formatPath(m_newname);

  try{

  if(isOldAbsoulute){
    if(isNewAbsoulute){
      std::filesystem::rename(path.getDrive()+n_oldname,path.getDrive()+n_newname);
    }else{
      std::filesystem::rename(path.getDrive()+n_oldname,path.getFullPath()+n_newname);
    }
  }else{
    if(isNewAbsoulute){
      std::filesystem::rename(path.getFullPath()+n_oldname,path.getDrive()+n_newname);
    }else{
      std::filesystem::rename(path.getFullPath()+n_oldname,path.getFullPath()+n_newname);
    }
  }
    return true;

  }catch(const std::filesystem::filesystem_error& e){
      std::cout<<"error occoured while renaming "<<e.what()<<std::endl;
      return false;
  }

}


int getSize(const std::string& path, const std::string& filename) {
    try {
        return std::filesystem::file_size(path + "/" + removeExtras(filename));
    }
    catch (const std::filesystem::filesystem_error& ex) {
        // Handle the file system error
        std::cerr << "File system error: " << ex.what() << std::endl;
        std::cerr << "Path involved: " << ex.path1().string() << std::endl;
        std::cerr << "Error code: " << ex.code().value() << " - " << ex.code().message() << std::endl;
        // Return -1 to indicate error
        return -1;
    }
    catch (const std::exception& ex) {
        // Handle other exceptions
        std::cerr << "Exception: " << ex.what() << std::endl;
        // Return -1 to indicate error
        return -1;
    }
}

void getDataClient(Client** client,ServerSocket* socket,std::mutex& m_mutex){
    
    *client = socket->getClient(); 

    if(*client != nullptr){
      std::cout<<"getDataClient : got a client for "<<(*client)->getId()<<std::endl;
    }else{
      std::cout<<"getDataClient : socket->getclient() returned with nullptr"<<std::endl;
    }

    m_mutex.unlock();
    std::cout<<"mutex unlocked from getDataClient"<<std::endl;
}

void SocketsPointerCleaner(Client** dataClient,ServerSocket** dataSocket){

    if(*dataClient != nullptr){
      std::cout<<"dataClient is non null deleting old client"<<std::endl;
      (*dataClient)->close();
      *dataClient = nullptr;
    }
    if(*dataSocket != nullptr){
      std::cout<<"dataSocket is non null deleting old Socket"<<std::endl;
      delete *dataSocket;
      *dataSocket = nullptr;
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

    ServerSocket* dataSocket = nullptr;
    Client* dataClient = nullptr;
    size_t offset = 0;
    std::string renameBuffer;
    std::mutex mutex;

    client->write(ready);

    Browze path("G:","/");

    while(true){
      int readData;
      std::string fullCommand = client->read(readData);
      std::string command = getCode(fullCommand.c_str());
      std::string subCommand = getCommand(fullCommand.c_str());

      std::transform(command.begin(), command.end(),command.begin(), [](unsigned char c){ return std::toupper(c);});

      if(readData <= 0){
        SocketsPointerCleaner(&dataClient,&dataSocket);
        std::cout<<"client's main connection disconnected closing serviceWorker"<<std::endl;
        break;
      }

      std::cout<<fullCommand<<std::endl;

      if(command  == "USER"){
        std::cout<<"username is correct asking for password"<<std::endl;
        client->write(askPassword);
      }else if

      (command  == "PASS"){
        std::cout<<"password is corrrect asking for method"<<std::endl;
        client->write(loggedIn);
      }else if

      (command == "TYPE"){
        std::cout<<"client want's type binary accepting"<<std::endl;
        client->write(ok);
      }else if

      (command == "SIZE"){
        std::cout<<"client is asking for size sending details"<<std::endl;
        int size = getSize(path.getFullPath(),subCommand);
        client->write((std::string("213 ")+std::to_string(size)+"\r\n").c_str());
      }else if

      (command == "FEAT"){
        std::cout<<"client is asking for features, sending not found"<<std::endl;
        client->write(syntaxError);
      }else if

      (command == "MKD"){
        std::cout<<"client is asking for creating a directory. Creating.."<<std::endl;
        if(mkd(removeExtras(subCommand),path)){
          client->write(("257 \""+removeExtras(subCommand)+"\": created.\r\n").c_str());
        }else{
          client->write(("550 "+removeExtras(subCommand)+" already exists.\r\n").c_str());
        }
      }else if
      
      (command == "RNFR"){
        std::cout<<"client is asking for renaming a file stored into renamebuffer"<<std::endl;
        renameBuffer = removeExtras(subCommand);
        client->write(rnfr);
      }else if

      (command == "RNTO"){
        std::cout<<"client is asking for rnto renaming a file to.."<<std::endl;
        if(renameFile(renameBuffer,removeExtras(subCommand),path)){
         client->write(rnto);
        }else{
         client->write(rn_error);
        }
        renameBuffer = "";
      }else if

      (command == "SYST"){
        std::cout<<"client is asking for system, sending windows system"<<std::endl;
        client->write(systemType);
      }else if

      (command == "CWD"){
        if(isAbsolutePath(subCommand)){
          path.setPath(removeExtras(subCommand).c_str());
        }else{
          path.to(removeExtras(subCommand).c_str());
        }
        std::cout<<"client is asking for changing directory changing to "<<path.getFullPath()<<std::endl;
        client->write((directoryChanged+path.getPath()+"\r\n").c_str());
      }else if

      (command == "CDUP"){
        std::cout<<"path before cdup is "<<path.getFullPath()<<std::endl;
        path.up();
        client->write((directoryChanged+path.getPath()+"\r\n").c_str());
        std::cout<<"command cdup executed the path is now "<<path.getPath()<<std::endl;
      }else if

      (command == "PWD"){
        client->write(("257 \""+path.getPath()+"\"is the current directory\r\n").c_str());
        std::cout<<"client is asking the current working directory sending "<<path.getPath()<<std::endl; 
      }else if

      (command == "PASV"){
        std::cout<<"starting passive mode"<<std::endl;
        SocketsPointerCleaner(&dataClient,&dataSocket);
        dataSocket = new ServerSocket(port);
        dataSocket->start();
        mutex.lock();
        std::cout<<"mutex locked from PASV and sent to getDataClient"<<std::endl;
        std::thread worker(getDataClient,&dataClient,dataSocket,std::ref(mutex));
        worker.detach();
        int p1 = port/256;
        int p2 = port%256;
        client->write((std::string(passiveMode)+std::to_string(p1)+","+std::to_string(p2)+")\r\n").c_str());
        port++;
      }else if

      (command == "EPSV"){
        std::cout<<"starting epsv mode"<<std::endl;
        SocketsPointerCleaner(&dataClient,&dataSocket);
        dataSocket = new ServerSocket(port);
        dataSocket->start();
        mutex.lock();
        std::cout<<"mutex locked from EPSV and sent to getDataClient"<<std::endl;
        std::thread worker(getDataClient,&dataClient,dataSocket,std::ref(mutex));
        worker.detach();
        client->write((std::string(ePassiveMode)+std::to_string(port)+std::string("|)\r\n")).c_str());
        port++;
      }else if

      (command == "NLST"){
        if(dataSocket == nullptr || dataClient == nullptr){
           client->write("503 PORT or PASV must be issued first\r\n");
        
        }else{
          std::cout<<"asked for nlst sending list"<<std::endl;
          client->write(fileStatusOkay);
          
          mutex.lock();
          dataClient->write(nlst(path.getFullPath().c_str()).c_str());
          std::cout<<"list sent succesfully.."<<std::endl;
          dataClient->close();
          dataClient = nullptr;
          client->write(closingDataConnection);
          std::cout<<"passive data connection closed"<<std::endl;
          mutex.unlock();
        }
      }else if

      (command == "MLSD"){
        if(dataSocket == nullptr || dataClient == nullptr){
           client->write("503 PORT or PASV must be issued first\r\n");
        }else{
            std::cout<<"asked MLSD sending mlsdList"<<std::endl;
            client->write(fileStatusOkay);
            if(subCommand.length() > 5){
            std::string newFilePath ;//= subCommand.substr(subCommand.find(" ")+1,subCommand.length());
            newFilePath = removeExtras(subCommand);
            path.setPath(newFilePath.c_str());
            }else{
            //path.setPath("/");
            }
            std::cout<<"the mlsd path after is "<<path.getFullPath()<<std::endl;
            mutex.lock();
            dataClient->write(mlsd(path.getFullPath().c_str()).c_str());
            std::cout<<"list sent succesfully.."<<std::endl;
            dataClient->close(); 
            dataClient = nullptr;

            client->write(closingDataConnection);
            std::cout<<"passive data connection closed"<<std::endl;
            mutex.unlock();
        }
      }else if

      (command == "LIST"){
        if(dataSocket == nullptr){
           client->write("503 PORT or PASV must be issued first\r\n");
        
        }else{
            std::cout<<"asked LIST sending file list.."<<std::endl;
            client->write(fileStatusOkay);

            if(subCommand.length() > 5){
              std::cout<<"absolute true setting absolute path"<<std::endl;
              std::string newFilePath = subCommand.substr(subCommand.find(" ")+1,subCommand.length());
              newFilePath = removeExtras(newFilePath);
              path.setPath(newFilePath.c_str());
            }else if(subCommand.find("-a") != std::string::npos){
              path.setPath("/");
            }
            

              mutex.lock();
              std::cout<<"mutex locked from LIST"<<std::endl;
              std::cout<<path.getFullPath()<<" is the list path"<<std::endl;
              dataClient->write(list(path.getFullPath().c_str()).c_str());
              std::cout<<"list sent succesfully.."<<std::endl;
              dataClient->close(); 
              dataClient = nullptr;

              client->write(closingDataConnection);
              std::cout<<"passive data connection closed"<<std::endl;
              mutex.unlock();
              std::cout<<"mutex unlocked from LIST"<<std::endl;
        }
      }else if

      (command == "RETR"){
        std::cout<<"asking for a file sending file to the client"<<std::endl;
        std::string retrPath;
        if(isAbsolutePath(subCommand)){
          retrPath = path.getDrive()+"/"+removeExtras(subCommand);
        }else{
          retrPath = path.getFullPath()+"/"+removeExtras(subCommand);
        }
        std::cout<<"the RETR path is "<<retrPath<<std::endl;

        if(std::filesystem::exists(retrPath)){
            if(std::filesystem::is_directory(retrPath)){
              client->write(("550 "+retrPath+": Not a plain file\r\n").c_str());
              std::cout<<"file not sent because it is a directory"<<std::endl;
            }else{
              client->write(fileStatusOkay);
                

                mutex.lock();
                std::cout<<"mutex locked from RETR"<<std::endl;
                sendFile(dataClient,retrPath,offset);
                std::cout<<"file sent succesfully.."<<std::endl;
                offset = 0;
                dataClient->close();  
                dataClient = nullptr;

                client->write(transferComplete);
                std::cout<<"passive data connection closed"<<std::endl;
                mutex.unlock();
                std::cout<<"mutex unlocked from RETR"<<std::endl;
            }

        }else{
          client->write(("550 "+retrPath+": No such file or directory\r\n").c_str());
              std::cout<<"file not sent because it doesn't exists"<<std::endl;
        }


      }else if
      
      (command == "STOR"){
          std::cout<<"client want's to store a file running store"<<std::endl;
          std::string storPath;
          std::string checkPath;
          if(isAbsolutePath(subCommand)){
            storPath = path.getDrive()+"/"+removeExtras(subCommand);
            checkPath = storPath.substr(0,storPath.rfind("/"));
          }else{
            storPath = path.getFullPath()+"/"+removeExtras(subCommand);
            checkPath = path.getFullPath();
          }

            if(std::filesystem::exists(checkPath)){
                if(std::filesystem::is_directory(storPath)){
                  client->write(("550 "+storPath+": Not a plain file\r\n").c_str());
                  std::cout<<"file not recieved because the filename is a directory"<<std::endl;
                }else{
                  client->write(fileStatusOkay);

                  mutex.lock();
                  recieveFile(dataClient,storPath,offset);
                  std::cout<<"file recieved succesfully.."<<std::endl;
                  offset = 0;
                  dataClient->close();  
                  dataClient = nullptr;

                  client->write(transferComplete);
                  std::cout<<"passive data connection closed"<<std::endl;
                  mutex.unlock();
                }

            }else{
              client->write(("550 "+storPath+": No such file or directory\r\n").c_str());
                  std::cout<<"file not sent because it doesn't exists"<<std::endl;
            }
      }else if

      (command == "REST"){
        offset = std::atoi(removeExtras(subCommand).c_str());
        std::cout<<"asking for REST setting offset value "<<std::to_string(offset)<<std::endl;
        client->write(("350 Restarting at "+std::to_string(offset)+". Send STORE or RETRIEVE to initiate transfer.\r\n").c_str());
      }else if

      (command == "NOOP"){
        std::cout<<"asking for SITE sending not ok"<<std::endl;
        client->write(ok);
      }else if

      (command == "OPTS"){
        std::cout<<"asking for options sending not ok"<<std::endl;
        client->write(ok);
      }else if

      (command == "ABOR"){
        std::cout<<"asking for abortion aborting.."<<std::endl;
          if(dataClient != nullptr){
             dataClient->close();
             dataClient = nullptr;
          }
          if(dataSocket != nullptr){
             delete dataSocket;
             dataSocket = nullptr;
          }
        client->write(abortSuccesfull);
      }else if

      (command == "QUIT"){
        std::cout<<"asked to quit the server quitting..."<<std::endl;
        SocketsPointerCleaner(&dataClient,&dataSocket);
        client->write("goodbye\r\n");
        break;
      }else{
        std::cout<<"no command "<<command<<" found sending not implemented"<<std::endl;
        client->write(notImplemented);
      }
    }

    std::cout<<"closing serviceWorker.."<<std::endl;
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
