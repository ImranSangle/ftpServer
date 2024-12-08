#!/bin/bash

#------------------function definitions------------------------

help(){
  local macro="(auto builds and compiles)."
  echo "Available Commands:"
  echo "init         Create a new cmake based project."
  echo "build        Build the cmake project."
  echo "compile      Compile all the executables and libraries."
  echo "test         Test the project with ctest $macro"
  echo "install      Install the project if cmake install command is used $macro"
  echo "debug        Debug the project with gdb debugger (if installed) $macro"
  echo "run          Build,compile and run the project."
  echo "reconfigure  Reconfigure the variables used for building the project by the run file."
  echo "clean        Remove the cmake build directory where all the files reside."
}

create_cmakelist(){
  echo "cmake_minimum_required(VERSION ${CMAKE_MIN_VERSION})" >> CMakeLists.txt
  echo "project(${PROJECT})" >> CMakeLists.txt
  echo "add_executable(${EXENAME} src/${EXENAME}.cpp)" >> CMakeLists.txt
}

create_main(){
  echo "int main(int argc,char* argv[]){" >> src/main.cpp
  echo "" >> src/main.cpp
  echo "return 0;" >> src/main.cpp
  echo "}" >> src/main.cpp
}

init(){
  if [[ ! -e "./src" ]] ; then
     mkdir src
     create_main
  fi
  if [[ ! -e "./include" ]] ; then
     mkdir include
  fi
  if [[ ! -e "./test" ]] ; then
     mkdir test
  fi
  if [[ ! -e "./CMakeLists.txt" ]] ; then
     create_cmakelist 
  fi
  echo "cmake based project directory structure created!"
}

build(){
  if [[ $DEBUG == "true" ]]; then
    echo "Making debug build."
    cmake -S . -B build -G "Unix Makefiles" -D CMAKE_EXPORT_COMPILE_COMMANDS=1 -D CMAKE_BUILD_TYPE=Debug
  else
    cmake -S . -B build -G "Unix Makefiles" -D CMAKE_EXPORT_COMPILE_COMMANDS=1 -D CMAKE_BUILD_TYPE=Release
  fi
}

compile(){
  if [[ -n ./build ]] ; then
    build
  fi
  cd build
  make
  if [[ $TEST == "true" ]] ; then
  ctest -VV
  fi
}

test(){
  if [[ -e ./build ]] ; then
    compile
  else
    build
    compile
  fi
  ctest -VV
}

install(){
  if [[ -e ./build ]] ; then
    compile
  else
    build
    compile
  fi
  make install
}

run(){
  if [[ -e ./build ]] ; then
    compile && exec ./$EXENAME
  else
    build
    compile && exec ./$EXENAME
  fi
}

debug(){
  if [[ -e ./build ]] ; then
    compile
  else
    build
    compile
  fi
  gdb $EXENAME
}

clean(){
  if [[ -e ./build ]] ; then
    rm -r ./build
  fi
}

config(){
  if [[ -n $1 && -n $2 ]] ; then
     echo "$1=$2" >> ./runconfig
   else
    echo "incorrect argument provided!"
  fi
}

configure(){
  echo "Enter your project name (Default=$PROJECT)."
  read VALUE
  if [[ -z $VALUE ]] ; then
    config PROJECT $PROJECT
  else
    config PROJECT $VALUE
  fi

  echo "Enter cmake minimum required version (Default=3.5)."
  read VALUE
  if [[ -z $VALUE ]] ; then
    config CMAKE_MIN_VERSION "3.5"
  else
    config CMAKE_MIN_VERSION $VALUE
  fi

  echo "Enter your executable name (Default=$EXENAME)."
  read VALUE
  if [[ -z $VALUE ]] ; then
    config EXENAME $EXENAME
  else
    config EXENAME $VALUE
  fi

  echo "Do you want to run tests after compiling? [y/N]"
  read VALUE
  if [[ $VALUE == "y" || $VALUE == "Y" ]] ; then
   config TEST true
  else
   config TEST false
  fi

  echo "Do you want to build this project as debug build? [Y/n]"
  read VALUE
  if [[ $VALUE == "n" || $VALUE == "N" ]] ; then
   config DEBUG false
  else
   config DEBUG true
  fi
}

reconfigure(){
  rm ./runconfig
  clean
  touch runconfig
  configure
  echo "new configurations:"
  cat ./runconfig
}

#----------- entry--------------

if [[ ! -e "./runconfig" ]] ; then
  touch runconfig
  configure
fi

source ./runconfig

if [[ -n $1 ]] ; then
  $1 $2 $3
else
 run
fi

