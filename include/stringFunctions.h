#pragma once

#include <string>

class Browze{
private:
  std::string path;
  std::string drive;
public:
  Browze(const char*,const char* m_path);

  std::string getPath() const ;

  std::string getFullPath() const ;

  std::string getDrive() const ;
  
  void setPath(const char*);
  
  void setDrive(const char*);

  void to(const char* m_name);

  void up();
};

void getWordAt(const char*,char*,int);


std::string getCode(const char*);

std::string getCommand(const char*);
