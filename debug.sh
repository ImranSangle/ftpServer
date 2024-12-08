echo starting debugger...
cd build
make ftpServer
gdb ./ftpServer -ex "set disassembly-flavor intel" -x ../gdbconfig.txt
