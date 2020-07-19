#include "server.h"

int main(int argc, char *argv[]) {
    if (argc > 1) {
        // Accept the address and port for work server
        std::string strArgv;
        std::size_t pos;

        strArgv = argv[2];
        int port = std::stoi(strArgv, &pos);
        if (pos < strArgv.size()) {
            std::cerr << "Trailing characters after number: " << strArgv
                      << std::endl;
        }

        strArgv = argv[3];
        int timeoutMinute = std::stoi(strArgv, &pos);
        if (pos < strArgv.size()) {
            std::cerr << "Trailing characters after number: " << strArgv
                      << std::endl;
        }

        Server server(argv[1], port, timeoutMinute);
        server.startServer();
    } else {
      // Strart the server at the default address
        Server server;
        server.startServer();
    }
}