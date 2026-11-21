#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "log.h"
#include "macros.h"
#include "os_common.h"
#include "rfc_3659.h"
#include "rfc_959.h"
#include "sockets.h"
#include "stringFunctions.h"
#include "users.h"

#define DATA_START_PORT 3001
#define DATA_END_PORT 3101

// status codes--------------------------------------------
static const char* ready = "220 service ready for new user\r\n";
static const char* askPassword = "331 username okay, password for ";
static const char* loggedIn = "230 logged in, proceed\r\n";
static const char* ok = "200 command type okay\r\n";
static const char* syntaxError = "500 syntax error\r\n";
static const char* systemTypeWindows = "215 windows_NT\r\n";
static const char* systemTypeLinux = "215 UNIX Type: Apache FtpServer\r\n";
static const char* passiveMode = "227 entering passive mode ";
static const char* directoryChanged = "250 directory changed to ";
static const char* ePassiveMode = "229 Entering Passive Mode (|||";
static const char* fileStatusOkay = "150 File status okay; about to open data connection\r\n";
static const char* closingDataConnection = "226 Closing data connection\r\n";
static const char* abortSuccesfull = "226 ABOR command succesfull\r\n";
static const char* notImplemented = "502 command not implemented\r\n";
static const char* transferComplete = "226 transfer complete\r\n";
static const char* rnfr = "350 Requested file action pending further information\r\n";
static const char* rnto = "250 Requested file action okay, file renamed\r\n";
static const char* rn_error = "503 Can't find the file which has to be renamed.\r\n";
static const char* relog = "503 Log in with user first\r\n";
static const char* cant_open_data_conn = "425 Can't open data connection.\r\n";
static const char* authentication_failed = "530 Authentication failed.\r\n";

std::string make_absolute_path(const Browze& browser, const std::string& l_path) {

    if (l_path[0] == '/') {
        return browser.getDrive() + browser.getPrefixPath() + l_path;
    }

    if (browser.getTrueFullPath()[browser.getTrueFullPath().length() - 1] == '/') {
        return browser.getTrueFullPath() + l_path;
    }

    return browser.getTrueFullPath() + "/" + l_path;
}

bool path_exists(const std::string& l_path) {

    FSTRY

    if (std::filesystem::exists(l_path)) {
        return true;
    }

    FSCATCH

    return false;
}

std::pair<std::string, std::string> parse_list_subcommand(const std::string& l_subcommand) {

    std::istringstream iss(l_subcommand);
    std::string token;
    std::string flags;
    std::string path;
    bool path_started = false;

    while (iss >> token) {

        if (!path_started && !token.empty() && token[0] == '-') {
            flags += token.substr(1);
        } else {

            if (!path.empty())
                path += " ";

            path += token;
            path_started = true;
        }
    }

    return {flags, path};
}

std::string removeExtras(const std::string& value) {

    std::string m_fileName;
    size_t findResult = value.find("\r");

    if (findResult == std::string::npos) {
        findResult = value.find("\n");
        if (findResult == std::string::npos) {
            m_fileName = value.substr(0, value.length());
        } else {
            m_fileName = value.substr(0, findResult);
        }
    } else {
        m_fileName = value.substr(0, findResult);
    }
    return m_fileName;
}

bool isAbsolutePath(const std::string& m_path) {

    if (m_path[0] == '/') {
        return true;
    }
    return false;
}

void getDataClient(std::unique_ptr<Client>& client, const std::unique_ptr<ServerSocket>& socket, std::weak_ptr<std::mutex> m_mutex) {

    try {

        client = socket->getClient();

        LOG("getDataClient : got a dataClient with id " << client->getId());

    } catch (const std::runtime_error& error) {
        LOG("failed to get dataClient Reason: " << error.what());
    }

    if (auto l_mutex = m_mutex.lock()) {
        l_mutex->unlock();
        LOG("mutex unlocked from getDataClient");
    }
}

void SocketsPointerCleaner(Client** dataClient, ServerSocket** dataSocket) {

    if (*dataClient != nullptr) {
        LOG("dataClient is non null deleting old client");
        (*dataClient)->close();
        *dataClient = nullptr;
    }
    if (*dataSocket != nullptr) {
        LOG("dataSocket is non null deleting old Socket");
        delete *dataSocket;
        *dataSocket = nullptr;
    }
}

void session_thread(std::unique_ptr<Client> client) {

    std::unique_ptr<ServerSocket> dataSocket;
    std::unique_ptr<Client> dataClient;
    int port;
    size_t offset = 0;
    std::string ipAddress = getIpAddress();
    std::string renameBuffer;
    std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();
    User current_session;

    client->write(ready);

    Browze& path = current_session.path;

    while (true) {

        int readData;
        std::string fullCommand = client->read(readData);
        std::string command = getCode(fullCommand);
        std::string subCommand = removeExtras(getCommand(fullCommand));

        std::transform(command.begin(), command.end(), command.begin(), [](unsigned char c) { return std::toupper(c); });

        if (readData <= 0) {
            LOG("client's main connection disconnected closing session_thread");
            break;
        }

        LOG(fullCommand);

        //-------------------------------------------RFC_959--------------------------------------------------------

        if (command == "USER") {

            if (current_session.is_authenticated) {
                client->write("230 Already logged-in!\r\n");
                continue;
            }

            LOG("username is okay asking for password");
            current_session.username = subCommand;
            current_session.failed_attempt = false;
            client->write((askPassword + current_session.username + "\r\n").c_str());

        } else if (command == "PASS") {

            if (current_session.is_authenticated) {
                client->write("230 Already logged-in!\r\n");
                continue;
            }

            if (current_session.failed_attempt) {
                LOG("Previous attempt was failed asking client to relog.");
                client->write(relog);
                continue;
            }

            current_session.password = subCommand;
            authenticate(current_session);
            if (current_session.is_authenticated) {
                LOG("password is corrrect asking for method");
                client->write(loggedIn);
            } else {
                LOG("authentication failed!");
                current_session.failed_attempt = true;
                client->write(authentication_failed);
            }

        } else if (command == "TYPE") {

            AUTH_CHECK

            LOG("client want's type binary accepting");
            client->write(ok);

        } else if (command == "MKD") {

            AUTH_CHECK

            if(!current_session.writable){
              LOG("client is asking to make directory in a non writable session, Declined!");
              client->write(("550 No permission to create "+subCommand+".\r\n").c_str());
              continue;
            }

            LOG("client is asking for creating a directory. Creating..");

            if (mkd(make_absolute_path(path, subCommand))) {
                client->write(("257 \"" + subCommand + "\": created.\r\n").c_str());
            } else {
                client->write(("550 " + subCommand + " already exists.\r\n").c_str());
            }

        } else if (command == "RNFR") {

            AUTH_CHECK

            LOG("client is asking for renaming a file stored into renamebuffer");
            renameBuffer = make_absolute_path(path, subCommand);
            client->write(rnfr);

        } else if (command == "RNTO") {

            AUTH_CHECK

            if(!current_session.writable){
              LOG("client  is asking to rename file/folder in a non writable session. Declined!");
              client->write("553 No permission to rename.\r\n");
              renameBuffer.clear();
              continue;
            }

            LOG("client is asking for rnto renaming a file to..");
            if (renameFile(renameBuffer, make_absolute_path(path, subCommand))) {
                client->write(rnto);
            } else {
                client->write(rn_error);
            }
            renameBuffer = "";

        } else if (command == "SYST") {

            AUTH_CHECK

#ifdef WIN64
            LOG("client is asking for system, sending windows system");
            client->write(systemTypeWindows);
#endif
#ifdef __linux__
            LOG("client is asking for system, sending linux system");
            client->write(systemTypeLinux);
#endif

        } else if (command == "CWD") {

            AUTH_CHECK

            std::string absolute_path = make_absolute_path(path, subCommand);

            if (path_exists(absolute_path)) {

                if (isAbsolutePath(subCommand)) {
                    path.setPath(subCommand.c_str());
                } else {
                    path.to(subCommand.c_str());
                }

                LOG("from CWD : client is asking for changing directory changing to " << path.getTrueFullPath());
                client->write((directoryChanged + path.getPath() + "\r\n").c_str());

            } else {
                LOG("from CWD : No such directory found " << subCommand);
                client->write("550 No such directory\r\n");
            }

        } else if (command == "CDUP") {

            AUTH_CHECK

            LOG("path before cdup is " << path.getTrueFullPath());
            path.up();
            client->write((directoryChanged + path.getPath() + "\r\n").c_str());
            LOG("command cdup executed the path is now " << path.getTrueFullPath());

        } else if (command == "PWD") {

            AUTH_CHECK

            client->write(("257 \"" + path.getPath() + "\"is the current directory\r\n").c_str());
            LOG("client is asking the current working directory sending " << path.getTruePath());

        } else if (command == "PASV") {

            AUTH_CHECK

            LOG("starting passive mode");

            try {
                dataSocket = std::make_unique<ServerSocket>(DATA_START_PORT, DATA_END_PORT);
                dataSocket->start();
                port = dataSocket->getPort();
            } catch (const std::runtime_error& error) {
                LOG("Failed to create a socket for data connection, Reason: " << error.what());
                client->write(cant_open_data_conn);
                continue;
            }

            mutex->lock();
            LOG("mutex locked from PASV and sent to getDataClient");
            std::thread worker(getDataClient, std::ref(dataClient), std::ref(dataSocket), std::weak_ptr<std::mutex>(mutex));
            worker.detach();
            int p1 = port / 256;
            int p2 = port % 256;
            client->write((std::string(passiveMode) + "(" + ipAddress + "," + std::to_string(p1) + "," + std::to_string(p2) + ")\r\n").c_str());

        } else if (command == "NLST") {

            AUTH_CHECK

            if (!dataSocket) {
                client->write("503 PORT or PASV must be issued first\r\n");
            } else {
                LOG("asked for nlst sending list");
                client->write(fileStatusOkay);

                mutex->lock();
                dataClient->write(nlst(make_absolute_path(path, subCommand)).c_str());
                LOG("list sent succesfully..");
                dataClient.reset();
                client->write(closingDataConnection);
                LOG("passive data connection closed");
                mutex->unlock();
            }

        } else if (command == "LIST") {

            AUTH_CHECK

            if (!dataSocket) {
                client->write("503 PORT or PASV must be issued first\r\n");

            } else {
                LOG("asked LIST sending file list..");

                std::pair<std::string, std::string> parsed_subcommand = parse_list_subcommand(subCommand); // todo: implement flags

                std::string finalListPath = make_absolute_path(path, parsed_subcommand.second);

                if (path_exists(finalListPath)) {
                    client->write(fileStatusOkay);
                    mutex->lock();
                    LOG("mutex locked from LIST");
                    LOG(finalListPath << " is the list path");
                    dataClient->write(list(finalListPath.c_str()).c_str());
                    LOG("list sent succesfully..");
                    dataClient.reset();

                    client->write(closingDataConnection);
                    LOG("passive data connection closed");
                    mutex->unlock();
                    LOG("mutex unlocked from LIST");
                } else {
                    LOG("from LIST : sending 450 Non existing file to the client");
                    client->write("450 Non existing file\r\n");
                }
            }

        } else if (command == "RETR") {

            AUTH_CHECK

            if (!dataSocket) {
                client->write("503 PORT or PASV must be issued first\r\n");
            } else {

                LOG("asking for a file sending file to the client");

                std::string absolute_path = make_absolute_path(path, subCommand);

                LOG("the RETR path is " << absolute_path);

                if (path_exists(absolute_path)) {

                    if (std::filesystem::is_directory(absolute_path)) {
                        client->write(("550 " + absolute_path + ": Not a plain file\r\n").c_str());
                        LOG("file not sent because it is a directory");
                    } else {
                        client->write(fileStatusOkay);

                        mutex->lock();
                        LOG("mutex locked from RETR");
                        retr(dataClient, absolute_path, offset);
                        LOG("file sent succesfully..");
                        offset = 0;
                        dataClient.reset();

                        client->write(transferComplete);
                        LOG("passive data connection closed");
                        mutex->unlock();
                        LOG("mutex unlocked from RETR");
                    }

                } else {
                    client->write(("550 " + absolute_path + ": No such file or directory\r\n").c_str());
                    LOG("file not sent because it doesn't exists");
                }
            }

        } else if (command == "STOR") {

            AUTH_CHECK

            if(!current_session.writable){
              LOG("client is asking to store a file in a non writable session. Declined!");
              client->write(("550 "+subCommand+": Permission denied.\r\n").c_str());
              continue;
            }

            if (!dataSocket) {
                client->write("503 PORT or PASV must be issued first\r\n");
            } else {

                LOG("client want's to store a file running store");

                std::string absolute_path = make_absolute_path(path, subCommand);
                std::string absolute_directory = absolute_path.substr(0, absolute_path.rfind('/'));

                if (path_exists(absolute_directory)) {

                    if (std::filesystem::is_directory(absolute_path)) {
                        client->write(("550 " + absolute_path + ": Not a plain file\r\n").c_str());
                        LOG("file not recieved because the filename is a directory");
                    } else {
                        client->write(fileStatusOkay);

                        mutex->lock();
                        stor(dataClient, absolute_path, offset);
                        LOG("file recieved succesfully..");
                        offset = 0;
                        dataClient.reset();

                        client->write(transferComplete);
                        LOG("passive data connection closed");
                        mutex->unlock();
                    }

                } else {
                    client->write(("550 " + absolute_path + ": No such file or directory\r\n").c_str());
                    LOG("file not sent because it doesn't exists");
                }
            }

        } else if (command == "NOOP") {

            AUTH_CHECK

            LOG("asking for NOOP sending ok");
            client->write(ok);

        } else if (command == "OPTS") {

            AUTH_CHECK

            LOG("asking for options sending not ok");
            client->write(ok);

        } else if (command == "ABOR") {

            AUTH_CHECK

            LOG("asking for abortion aborting..");
            dataClient.reset();
            client->write(abortSuccesfull);

        } else if (command == "QUIT") {

            LOG("asked to quit the server quitting...");
            client->write("goodbye\r\n");
            break;

            //-------------------------------------------RFC_2389-------------------------------------------------------

        } else if (command == "FEAT") {

            AUTH_CHECK

            LOG("client is asking for features, sending not found");
            client->write(syntaxError);

            //-------------------------------------------RFC_2428-------------------------------------------------------

        } else if (command == "EPSV") {

            AUTH_CHECK

            LOG("starting epsv mode..");

            try {
                dataSocket = std::make_unique<ServerSocket>(DATA_START_PORT, DATA_END_PORT);
                dataSocket->start();
                port = dataSocket->getPort();
            } catch (const std::runtime_error& error) {
                LOG("Failed to create a socket for data connection, Reason: " << error.what());
                client->write(cant_open_data_conn);
                continue;
            }

            mutex->lock();
            LOG("mutex locked from EPSV and sent to getDataClient");
            std::thread worker(getDataClient, std::ref(dataClient), std::ref(dataSocket), std::weak_ptr<std::mutex>(mutex));
            worker.detach();
            client->write((std::string(ePassiveMode) + std::to_string(port) + std::string("|)\r\n")).c_str());

            //-------------------------------------------RFC_3659-------------------------------------------------------

        } else if (command == "MDTM") {

            AUTH_CHECK

            std::string absolute_path = make_absolute_path(path, subCommand);

            if (path_exists(absolute_path)) {
                client->write(mdtm(absolute_path).c_str());
            } else {
                client->write("550 File not found\r\n");
            }

        } else if (command == "MLST") {

            AUTH_CHECK

            std::string absolute_path = make_absolute_path(path, subCommand);

            if (path_exists(absolute_path)) {
                LOG("from MLST : Sending mlst of the requested file " + subCommand);
                client->write(("250-\r\n" + mlst(absolute_path)).c_str());
                client->write("250 Requested file action okay, completed\r\n");
            } else {
                LOG("from MLST : Not a valid pathname");
                client->write("501 Not a valid pathname\r\n");
            }

        } else if (command == "MLSD") {

            AUTH_CHECK

            if (!dataSocket) {
                client->write("503 PORT or PASV must be issued first\r\n");
            } else {
                LOG("asked MLSD sending mlsdList");

                std::pair<std::string, std::string> parsed_subcommand = parse_list_subcommand(subCommand); // todo: implement flags

                std::string finalListPath = make_absolute_path(path, parsed_subcommand.second);

                if (path_exists(finalListPath)) {
                    client->write(fileStatusOkay);
                    LOG("the mlsd path after is " << finalListPath);
                    mutex->lock();
                    dataClient->write(mlsd(finalListPath).c_str());
                    LOG("list sent succesfully..");
                    dataClient.reset();

                    client->write(closingDataConnection);
                    LOG("passive data connection closed");
                    mutex->unlock();

                } else {
                    LOG("from MLSD : sending 450 Non existing file to the client");
                    client->write("450 Non existing file\r\n");
                }
            }

        } else if (command == "REST") {

            AUTH_CHECK

            offset = std::stoll(subCommand.c_str());
            LOG("asking for REST setting offset value " << std::to_string(offset));
            client->write(("350 Restarting at " + std::to_string(offset) + ". Send STORE or RETRIEVE to initiate transfer.\r\n").c_str());

        } else if (command == "SIZE") {

            AUTH_CHECK

            LOG("client is asking for size sending details");
            size_t size = getSize(make_absolute_path(path, subCommand));
            if (size == -1) {
                client->write(("550 " + subCommand + ": No such file or directory\r\n").c_str());
            } else {
                client->write((std::string("213 ") + std::to_string(size) + "\r\n").c_str());
            }

        } else {

            AUTH_CHECK

            LOG("no command" << command << " found sending not implemented");
            client->write(notImplemented);
        }
    }

    LOG("closing session_thread..");
}

int main() {

#ifdef _WIN64
    load_wsa();
#endif

    load_user_data();

    ServerSocket socket(3000);

    socket.start();

    while (true) {

        LOG("main thread ready ready waiting for new connections...");

        std::unique_ptr<Client> client = socket.getClient();

        LOG("new client connected starting session_thread & sending service ready");

        std::thread worker(session_thread, std::move(client));

        worker.detach();
    }

#ifdef _WIN64
    unload_wsa();
#endif

    return 0;
}
