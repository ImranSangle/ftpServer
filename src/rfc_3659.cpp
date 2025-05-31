#include <string>
#include <filesystem>
#include <iostream>

#include "macros.h"

std::string mlst(const std::string& l_path){

    std::string file_name = l_path;

    if(file_name[file_name.length()-1] == '/' && file_name.length() > 1){
        file_name.pop_back();
    }

    file_name = file_name.substr(file_name.rfind('/')+1,file_name.length());

    if(std::filesystem::is_directory(l_path)){
        return "Size=0"+(";Modify=20240305134627;Type=dir; "+file_name)+"\r\n\r\n";
    }else{
        return "Size="+std::to_string(std::filesystem::file_size(l_path))+";Modify=20240305134627;Type=file; "+file_name+"\r\n\r\n";
    }

}


std::string mlsd(const std::string& l_path){

    std::string names;

    FSTRY

    for(const auto &entry : std::filesystem::directory_iterator(l_path)){


        if(entry.is_directory()){
            names+= "Size=0;Modify=20240228145553.000;Type=dir; "+entry.path().filename().string();
        }else{
            names+= "Size="+std::to_string(entry.file_size())+";Modify=20240228145553.000;Type=file; "+entry.path().filename().string();
        }
        names+="\r\n";

    }

    FSCATCH

  return names;
}


int getSize(const std::string& l_path) {

    FSTRY
        return std::filesystem::file_size(l_path);
    FSCATCH
        return -1;
}
