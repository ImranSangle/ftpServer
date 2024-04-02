#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <algorithm>
#include <thread>
#include <filesystem>
#include <fstream>

#include "sockets.h"
#include "stringFunctions.h"
#include "log.h"


static int port = 3001;

//status codes--------------------------------------------
static const char* ready = "220 service ready for new user\r\n";
static const char* askPassword = "331 username okay, password for username\r\n";
static const char* loggedIn  = "230 logged in, proceed\r\n";
static const char* ok  = "200 command type okay\r\n";
static const char* syntaxError  = "500 syntax error\r\n";
static const char* systemTypeWindows  = "215 windows_NT\r\n";
static const char* systemTypeLinux  = "215 UNIX Type: Apache FtpServer\r\n";
static const char* passiveMode  = "227 entering passive mode ";
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

std::string formatListSubcommand(const std::string& m_subcommand){

  std::string result;

  size_t aPos = m_subcommand.find("-a");
  size_t lPos = m_subcommand.find("-l");

  if(aPos != std::string::npos){
    if(m_subcommand[aPos+2] == ' '){
      return m_subcommand.substr(aPos+3,m_subcommand.length()); 
    }else{
      return m_subcommand;
    }
  }else if(lPos != std::string::npos){
    if(m_subcommand[lPos+2] == ' '){
      return m_subcommand.substr(lPos+3,m_subcommand.length()); 
    }else{
      return m_subcommand;
    }
  }else{
    return m_subcommand;
  }
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
    if(!std::filesystem::is_directory(path)){
       names+= "-rw------- 1 owner owner "+std::to_string(std::filesystem::file_size(path))+" Feb 25 00:54 "+path;
       return names;
    }
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        try {

            if (entry.is_directory()) {
                names += "drwxrwxrwx 2 owner owner 4096 Feb 25 00:54 " + entry.path().filename().string();
            }
            else {
                names += "-rwxrwxrwx 1 owner owner " + std::to_string(entry.file_size()) + " Feb 25 00:54 " + entry.path().filename().string();
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
    if(client == nullptr){
       LOG("from sendfile() : client is nullptr returning.");
       return;
    }
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
                if(client->m_write(buffer, bytesReadThisRound) <=0){
                  LOG("from sendfile() : failed to send file breaking the loop.");
                  break;
                }
                bytesRead += bytesReadThisRound;
            }
        }

        // if (bytesRead != fileSize) {
        //     std::cout << "Error: File size mismatch."<<std::endl;
        // }

        input.close(); // Close the file after reading
    } else {
        LOG("Failed to open file from sendFile()");
    }
}

void recieveFile(Client* client,const std::string& m_path,size_t m_offset){
    std::ofstream output(m_path, std::ios::binary);

    if(output.is_open()){
       
      output.seekp(m_offset,std::ios::beg);

      const size_t bufferSize = 4096;
      char buffer[bufferSize];
      int dataRead = 1;
      
      while(dataRead > 0){
          dataRead = client->m_read(bufferSize,buffer);

          if(dataRead > 0){
           output.write(buffer,dataRead);
          }
      }

       output.close();

    }else{
       LOG("failed to send the file error from recieveFile()");
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

bool checkPath(const std::string& m_path,const Browze& path){

  std::string c_path = formatPath(m_path);

  if(isAbsolutePath(m_path)){
    return std::filesystem::exists(path.getDrive()+path.getPrefixPath()+c_path);
  }

  return std::filesystem::exists(path.getTrueFullPath()+c_path);

}

std::string mlst(const std::string& m_path,const Browze& path){

   std::string f_path = formatPath(m_path);
   std::string finalPath;

   if(isAbsolutePath(m_path)){
     finalPath = path.getDrive()+path.getPrefixPath()+f_path;
   }else{
     finalPath = path.getTrueFullPath()+f_path;
   }

   if(std::filesystem::is_directory(finalPath)){
     return "Size=0"+(";Modify=20240305134627;Type=dir;"+f_path)+"\r\n";
   }else{
     return "Size="+std::to_string(std::filesystem::file_size(finalPath))+";Modify=20240305134627;Type=file;"+f_path+"\r\n";
   }

}

bool mkd(const std::string& name,const Browze path){
     if(isAbsolutePath(name)){
        std::string c_name = formatPath(name);
       if(!std::filesystem::exists(path.getDrive()+path.getPrefixPath()+c_name)){
         if(std::filesystem::create_directory(path.getDrive()+path.getPrefixPath()+c_name)){
             LOG("created a new directory "<<name);
              return true;
         }

       }

    }else if(!std::filesystem::exists(path.getTruePath()+"/"+name)){
           if(std::filesystem::create_directory(path.getTruePath()+"/"+name)){
             LOG("created a new directory "<<name);
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
      std::filesystem::rename(path.getDrive()+path.getPrefixPath()+n_oldname,path.getDrive()+path.getPrefixPath()+n_newname);
    }else{
      std::filesystem::rename(path.getDrive()+path.getPrefixPath()+n_oldname,path.getTrueFullPath()+n_newname);
    }
  }else{
    if(isNewAbsoulute){
      std::filesystem::rename(path.getTrueFullPath()+n_oldname,path.getDrive()+path.getPrefixPath()+n_newname);
    }else{
      std::filesystem::rename(path.getTrueFullPath()+n_oldname,path.getTrueFullPath()+n_newname);
    }
  }
    return true;

  }catch(const std::filesystem::filesystem_error& e){
       LOG("error occoured while renaming "<<e.what());
      return false;
  }

}


int getSize(const std::string& path, const std::string& filename) {
    try {
        return std::filesystem::file_size(path+"/" +filename);
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

    if(socket->waitTill(2) == 0){

    *client = socket->getClient(); 

    }

    if(*client != nullptr){
       LOG("getDataClient : got a client for "<<(*client)->getId());
    }else{
       LOG("getDataClient : socket->getclient() returned with nullptr");
    }

    m_mutex.unlock();
     LOG("mutex unlocked from getDataClient");
}

void SocketsPointerCleaner(Client** dataClient,ServerSocket** dataSocket){

    if(*dataClient != nullptr){
       LOG("dataClient is non null deleting old client");
      (*dataClient)->close();
      *dataClient = nullptr;
    }
    if(*dataSocket != nullptr){
       LOG("dataSocket is non null deleting old Socket");
      delete *dataSocket;
      *dataSocket = nullptr;
    }
}

#ifdef _WIN64

void loadwsa(){
  WSADATA ws;
  if(WSAStartup(MAKEWORD(2,2),&ws) != 0){
       LOG("failed to load the dll closing...");
    exit(1);
  }
}

#endif

void serviceWorker(Client* client){

    ServerSocket* dataSocket = nullptr;
    Client* dataClient = nullptr;
    size_t offset = 0;
    std::string ipAddress = getIpAddress();
    std::string renameBuffer;
    std::mutex mutex;

    client->write(ready);

    #ifdef WIN64
    Browze path("F:","/");
    #endif

    #ifdef __linux__
      Browze path("","/");
      path.setPrefixPath("/storage/emulated/0");
    #endif

    while(true){
      int readData;
      std::string fullCommand = client->read(readData);
      std::string command = getCode(fullCommand.c_str());
      std::string subCommand = removeExtras(getCommand(fullCommand.c_str()));

      std::transform(command.begin(), command.end(),command.begin(), [](unsigned char c){ return std::toupper(c);});

      if(readData <= 0){
        SocketsPointerCleaner(&dataClient,&dataSocket);
         LOG("client's main connection disconnected closing serviceWorker");
        break;
      }

      LOG(fullCommand);

      if(command  == "USER"){
         LOG("username is correct asking for password");
        client->write(askPassword);
      }else if

      (command  == "PASS"){
         LOG("password is corrrect asking for method");
        client->write(loggedIn);
      }else if

      (command == "TYPE"){
         LOG("client want's type binary accepting");
        client->write(ok);
      }else if

      (command == "SIZE"){
         LOG("client is asking for size sending details");
        int size = getSize(path.getTrueFullPath(),subCommand);
        if(size == -1){
          client->write(("550 "+subCommand+": No such file or directory\r\n").c_str());
        }else{
          client->write((std::string("213 ")+std::to_string(size)+"\r\n").c_str());
        }
      }else if

      (command == "FEAT"){
         LOG("client is asking for features, sending not found");
        client->write(syntaxError);
      }else if

      (command == "MKD"){
         LOG("client is asking for creating a directory. Creating..");
        if(mkd(subCommand,path)){
          client->write(("257 \""+subCommand+"\": created.\r\n").c_str());
        }else{
          client->write(("550 "+subCommand+" already exists.\r\n").c_str());
        }
      }else if
      
      (command == "RNFR"){
         LOG("client is asking for renaming a file stored into renamebuffer");
        renameBuffer = subCommand;
        client->write(rnfr);
      }else if

      (command == "RNTO"){
         LOG("client is asking for rnto renaming a file to..");
        if(renameFile(renameBuffer,subCommand,path)){
         client->write(rnto);
        }else{
         client->write(rn_error);
        }
        renameBuffer = "";
      }else if

      (command == "SYST"){
        #ifdef WIN64
        LOG("client is asking for system, sending windows system");
        client->write(systemTypeWindows);
        #endif
        #ifdef __linux__
        LOG("client is asking for system, sending linux system");
        client->write(systemTypeLinux);
        #endif
      }else if

      (command == "CWD"){
        if(checkPath(subCommand,path)){
          if(isAbsolutePath(subCommand)){
            path.setPath(subCommand.c_str());
          }else{
            path.to(subCommand.c_str());
          }
          LOG("from CWD : client is asking for changing directory changing to "<<path.getTrueFullPath());
          client->write((directoryChanged+path.getPath()+"\r\n").c_str());
        }else{
          LOG("from CWD : No such directory found "<<subCommand);
          client->write("550 No such directory\r\n");
        }
      }else if

      (command == "CDUP"){
         LOG("path before cdup is "<<path.getTrueFullPath());
        path.up();
        client->write((directoryChanged+path.getPath()+"\r\n").c_str());
         LOG("command cdup executed the path is now "<<path.getTrueFullPath());
      }else if

      (command == "PWD"){
        client->write(("257 \""+path.getPath()+"\"is the current directory\r\n").c_str());
         LOG("client is asking the current working directory sending "<<path.getTruePath());
      }else if

      (command == "PASV"){
         LOG("starting passive mode");
        SocketsPointerCleaner(&dataClient,&dataSocket);
        dataSocket = new ServerSocket(port);
        dataSocket->start();
        mutex.lock();
         LOG("mutex locked from PASV and sent to getDataClient");
        std::thread worker(getDataClient,&dataClient,dataSocket,std::ref(mutex));
        worker.detach();
        int p1 = port/256;
        int p2 = port%256;
        client->write((std::string(passiveMode)+"("+ipAddress+","+std::to_string(p1)+","+std::to_string(p2)+")\r\n").c_str());
        port++;
      }else if

      (command == "EPSV"){
         LOG("starting epsv mode");
        SocketsPointerCleaner(&dataClient,&dataSocket);
        dataSocket = new ServerSocket(port);
        dataSocket->start();
        mutex.lock();
         LOG("mutex locked from EPSV and sent to getDataClient");
        std::thread worker(getDataClient,&dataClient,dataSocket,std::ref(mutex));
        worker.detach();
        client->write((std::string(ePassiveMode)+std::to_string(port)+std::string("|)\r\n")).c_str());
        port++;
      }else if

      (command == "NLST"){
        if(dataSocket == nullptr){
           client->write("503 PORT or PASV must be issued first\r\n");
        }else{
           LOG("asked for nlst sending list");
          client->write(fileStatusOkay);
          
          mutex.lock();
          dataClient->write(nlst(path.getTrueFullPath().c_str()).c_str());
           LOG("list sent succesfully..");
          dataClient->close();
          dataClient = nullptr;
          client->write(closingDataConnection);
           LOG("passive data connection closed");
          mutex.unlock();
        }
      }else if

      (command == "MLST"){
        if(checkPath(subCommand,path)){
          LOG("from MLST : Sending mlst of the requested file "+subCommand);
          client->write(("250-"+mlst(subCommand,path)).c_str());
          client->write("250 Requested file action okay, completed\r\n");
        }else{
         LOG("from MLST : Not a valid pathname");
         client->write("501 Not a valid pathname\r\n");
        }
      }else if

      (command == "MLSD"){
        if(dataSocket == nullptr){
           client->write("503 PORT or PASV must be issued first\r\n");
        }else{
            LOG("asked MLSD sending mlsdList");

            std::string formatedListPath = formatListSubcommand(subCommand);
            std::string finalListPath;

            if(formatedListPath.length() > 0){
               finalListPath = path.getDrive()+path.getPrefixPath()+formatPath(formatedListPath);
            }else{
               finalListPath = path.getTrueFullPath();
            }
            
            if(std::filesystem::exists(finalListPath)){
              client->write(fileStatusOkay);
              LOG("the mlsd path after is "<<finalListPath);
              mutex.lock();
              dataClient->write(mlsd(finalListPath.c_str()).c_str());
              LOG("list sent succesfully..");
              dataClient->close(); 
              dataClient = nullptr;

              client->write(closingDataConnection);
              LOG("passive data connection closed");
              mutex.unlock();

            }else{
              LOG("from MLSD : sending 450 Non existing file to the client");
              client->write("450 Non existing file\r\n");
            }
        }
      }else if

      (command == "LIST"){
        if(dataSocket == nullptr){
           client->write("503 PORT or PASV must be issued first\r\n");
        
        }else{
            LOG("asked LIST sending file list..");

            std::string formatedListPath = formatListSubcommand(subCommand);
            std::string finalListPath;

            if(formatedListPath.length() > 0){
               finalListPath = path.getDrive()+path.getPrefixPath()+formatPath(formatedListPath);
            }else{
               finalListPath = path.getTrueFullPath();
            }
            
            if(std::filesystem::exists(finalListPath)){
              client->write(fileStatusOkay);
              mutex.lock();
              LOG("mutex locked from LIST");
              LOG(finalListPath<<" is the list path");
              dataClient->write(list(finalListPath.c_str()).c_str());
              LOG("list sent succesfully..");
              dataClient->close(); 
              dataClient = nullptr;

              client->write(closingDataConnection);
              LOG("passive data connection closed");
              mutex.unlock();
              LOG("mutex unlocked from LIST");
            }else{
              LOG("from LIST : sending 450 Non existing file to the client");
              client->write("450 Non existing file\r\n");
            }

        }
      }else if

      (command == "RETR"){
         LOG("asking for a file sending file to the client");
        std::string retrPath;
        if(isAbsolutePath(subCommand)){
          retrPath = path.getDrive()+path.getPrefixPath()+"/"+subCommand;
        }else{
          retrPath = path.getTrueFullPath()+"/"+subCommand;
        }
         LOG("the RETR path is "<<retrPath);

        if(std::filesystem::exists(retrPath)){
            if(std::filesystem::is_directory(retrPath)){
              client->write(("550 "+retrPath+": Not a plain file\r\n").c_str());
             LOG("file not sent because it is a directory");
            }else{
              client->write(fileStatusOkay);
                

              mutex.lock();
              LOG("mutex locked from RETR");
              sendFile(dataClient,retrPath,offset);
              LOG("file sent succesfully..");
              offset = 0;
              dataClient->close();  
              dataClient = nullptr;

              client->write(transferComplete);
              LOG("passive data connection closed");
              mutex.unlock();
              LOG("mutex unlocked from RETR");
            }

        }else{
          client->write(("550 "+retrPath+": No such file or directory\r\n").c_str());
           LOG("file not sent because it doesn't exists");
        }


      }else if
      
      (command == "STOR"){
         LOG("client want's to store a file running store");
          std::string storPath;
          std::string checkPath;
          if(isAbsolutePath(subCommand)){
            storPath = path.getDrive()+path.getPrefixPath()+"/"+subCommand;
            checkPath = storPath.substr(0,storPath.rfind("/"));
          }else{
            storPath = path.getTrueFullPath()+"/"+subCommand;
            checkPath = path.getTrueFullPath();
          }

            if(std::filesystem::exists(checkPath)){
                if(std::filesystem::is_directory(storPath)){
                  client->write(("550 "+storPath+": Not a plain file\r\n").c_str());
                  LOG("file not recieved because the filename is a directory");
                }else{
                  client->write(fileStatusOkay);

                  mutex.lock();
                  recieveFile(dataClient,storPath,offset);
                  LOG("file recieved succesfully..");
                  offset = 0;
                  dataClient->close();  
                  dataClient = nullptr;

                  client->write(transferComplete);
                  LOG("passive data connection closed");
                  mutex.unlock();
                }

            }else{
              client->write(("550 "+storPath+": No such file or directory\r\n").c_str());
           LOG("file not sent because it doesn't exists");
            }
      }else if

      (command == "REST"){
        offset = std::atoi(subCommand.c_str());
         LOG("asking for REST setting offset value "<<std::to_string(offset));
        client->write(("350 Restarting at "+std::to_string(offset)+". Send STORE or RETRIEVE to initiate transfer.\r\n").c_str());
      }else if

      (command == "NOOP"){
         LOG("asking for NOOP sending ok");
        client->write(ok);
      }else if

      (command == "OPTS"){
         LOG("asking for options sending not ok");
        client->write(ok);
      }else if

      (command == "ABOR"){
         LOG("asking for abortion aborting..");
        SocketsPointerCleaner(&dataClient,&dataSocket);
        client->write(abortSuccesfull);
      }else if

      (command == "QUIT"){
         LOG("asked to quit the server quitting...");
        SocketsPointerCleaner(&dataClient,&dataSocket);
        client->write("goodbye\r\n");
        break;
      }else{
        LOG("no command"<<command<<" found sending not implemented");
        client->write(notImplemented);
      }
    }

     LOG("closing serviceWorker..");
    client->close();

}

int main(){

  #ifdef _WIN64
  loadwsa();
  #endif

  ServerSocket socket(3000);

  socket.start();

  while(true){
    LOG("main thread ready ready waiting for new connections...");

    Client* client = socket.getClient();

    LOG("new client connected starting serviceWorker & sending service ready");

    std::thread worker(serviceWorker,client);

    worker.detach();

  }

  #ifdef _WIN64
  WSACleanup();
  #endif

  return 0;
}
