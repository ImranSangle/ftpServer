#include <cstddef>
#include <stringFunctions.h>
#include <cstring>

void getWordAt(char* value,char* input,int index){
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
