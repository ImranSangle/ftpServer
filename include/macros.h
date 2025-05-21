#pragma once

#define FSDITRY try{

#define FSDICATCH }catch (const std::filesystem::filesystem_error& ex) {                            \
   std::cerr << "File system error: " << ex.what() << std::endl;                                    \
   std::cerr << "Path involved: " << ex.path1().string() << std::endl;                              \
   std::cerr << "Error code: " << ex.code().value() << " - " << ex.code().message() << std::endl;   \
   continue;                                                                                        \
   }                                                                                                \
   catch (const std::exception& ex) {                                                               \
       std::cerr << "Exception: " << ex.what() << std::endl;                                        \
       continue;                                                                                    \
   }

#define FSTRY try{

#define FSCATCH }catch (const std::filesystem::filesystem_error& ex) {                                \
     std::cerr << "File system error: " << ex.what() << std::endl;                                    \
     std::cerr << "Path involved: " << ex.path1().string() << std::endl;                              \
     std::cerr << "Error code: " << ex.code().value() << " - " << ex.code().message() << std::endl;   \
   }                                                                                                  \
   catch (const std::exception& ex) {                                                                 \
     std::cerr << "Exception: " << ex.what() << std::endl;                                            \
   }
