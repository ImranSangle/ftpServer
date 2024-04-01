#include <iostream>

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

int main(){

  std::cout<<formatListSubcommand("-a 0/storage/emulated/0/video.mp4")<<std::endl;

  return 0;
}
