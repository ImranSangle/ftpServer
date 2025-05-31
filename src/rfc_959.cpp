#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>

#include "log.h"
#include "macros.h"
#include "sockets.h"

std::string nlst(const std::string& l_path){

    std::string names;

    FSTRY

    for(const auto &entry : std::filesystem::directory_iterator(l_path)){

        names+=entry.path().filename().string();
        names+="\r\n";

    }

    FSCATCH

    return names;
}

std::string list(const std::string& l_path) {

    std::string names;

    FSTRY

    if(!std::filesystem::is_directory(l_path)){
        names+= "-rw------- 1 owner owner "+std::to_string(std::filesystem::file_size(l_path))+" Feb 25 00:54 "+l_path;
        return names;
    }

    for (const auto& entry : std::filesystem::directory_iterator(l_path)) {
    

        if (entry.is_directory()) {
            names += "drwxrwxrwx 2 owner owner 4096 Feb 25 00:54 " + entry.path().filename().string();
        }
        else {
            names += "-rwxrwxrwx 1 owner owner " + std::to_string(entry.file_size()) + " Feb 25 00:54 " + entry.path().filename().string();
        }
        names += "\r\n";
        

    }

    FSCATCH

    return names;
}

void retr(Client* client,const std::string& m_path,size_t m_offset) {

    if(client == nullptr){
        LOG("from sendfile() : client is nullptr returning.");
        return;
    }

    std::ifstream input(m_path, std::ios::binary);

    if (input.is_open()) {

        input.seekg(m_offset,std::ios::beg);

        const size_t bufferSize = 4096;
        char buffer[bufferSize];

        while (!input.eof()) {
            input.read(buffer, bufferSize);
            size_t bytesReadThisRound = input.gcount(); 

            if(client->m_write(buffer, bytesReadThisRound) <=0){
                LOG("from sendfile() : failed to send file breaking the loop.");
                break;
            }
        }

        input.close(); 
    } else {
        LOG("Failed to open file from sendFile()");
    }
}

void stor(Client* client,const std::string& m_path,size_t m_offset){
  
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

bool mkd(const std::string& l_path){
     
    FSTRY

        if(!std::filesystem::exists(l_path)){

            if(std::filesystem::create_directory(l_path)){
                LOG("created a new directory "<< l_path);
                return true;
            }

        }

    FSCATCH

    return false;
}

bool renameFile(const std::string& l_oldname,const std::string& l_newname){

    FSTRY

    std::filesystem::rename(l_oldname,l_newname);
      
    return true;

    FSCATCH

    return false;
}
