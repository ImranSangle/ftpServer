#include <iostream>
#include <filesystem>

class Browze{
private:
  std::string path;
public:
  Browze(const char* m_path){
     this->path = m_path;  
  }
  std::string getPath(){
    return this->path;
  }
  void to(const char* m_name){
    if(this->path[this->path.size()-1] != '/'){
      this->path+="/";
    }
    this->path+=m_name;
  }
  void up(){
   this->path = this->path.substr(0,this->path.rfind("/"));
  }
};

void listFiles(const char* path){
  for(const auto &entry : std::filesystem::directory_iterator(path)){
    if(entry.is_directory()){
      std::cout<<"directory : "<<entry.path().filename()<<std::endl;
    }else{
      std::cout<< entry.path().filename()<<std::endl;
    }
  }

}

class Object{
  public:
  int x;
  Object(){
    std::cout<<"object created"<<std::endl;
  }

  ~Object(){
    std::cout<<"object destroyed"<<std::endl;
  }
};

Object getObject(){

   Object x;
   Object y;

  return x;
}

int main(){
  

  return 0;
}
