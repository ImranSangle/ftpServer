@echo making buildfiles with debug symbols and logs enable..
cmake -S . -B build -G"Unix Makefiles" -D CMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug
@echo buildfiles generated successfully.
