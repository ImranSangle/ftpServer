#include <iostream>
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
