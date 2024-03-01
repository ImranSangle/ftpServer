#include <cstddef>
#include <stringFunctions.h>
#include <cstring>

//browze class definition------------------------------
Browze::Browze(const char* m_drive,const char* m_path){
     this->path = m_path;  
     this->drive = m_drive;
  }

  std::string Browze::getPath(){
    return this->path;
  }

  std::string Browze::getFullPath(){
    return this->drive+this->path;
  }

  std::string Browze::getDrive(){
    return this->drive;
  }
  
  void Browze::setPath(const char* m_path){
     this->path = m_path; 
  }
  
  void Browze::setDrive(const char* m_letter){
     this->drive = m_letter;
  }
  
  void Browze::to(const char* m_name){
    if(this->path[this->path.size()-1] != '/'){
      this->path+="/";
    }
    this->path+=m_name;
  }

  void Browze::up(){
   this->path = this->path.substr(0,this->path.rfind("/"));
  }

//browze end-----------------

void getWordAt(const char* value,char* input,int index){
  int spaceCount = 0;
  int length = strlen(value);
  int srcPosition = 0;

  for(int i=0;i<length;i++){
    if((value[i] == ' ' || value[i] == '\n') && spaceCount < index ){
      spaceCount++;
      continue;
    }
    if(spaceCount >= index){
      if(value[i] == ' ' || value[i] == '\n'){
	input[srcPosition] = 0;
	break;
      }
      input[srcPosition] = value[i];
      srcPosition++;
    }
  }

}

std::string getCode(const char* text){
  std::string value = text;
  size_t findResult = value.find(" ");
  if(value.find(" ") == std::string::npos){
    value = value.substr(0,value.length()-1);
  }else{
    value = value.substr(0,findResult);
  }

  return value;
}

std::string getCommand(const char* text){
   std::string value = text;  

   value = value.substr(value.find(" ")+1,value.size());

   return value;
}
