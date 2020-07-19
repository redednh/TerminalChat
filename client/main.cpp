#include "client.h"

int main(int argc, char* argv[]) {
    if (argc > 1) {
        // Accept the address and port for connect to the server
        std::string strArgv;
        std::size_t pos;

        strArgv = argv[2];
        int port = std::stoi(strArgv, &pos);
        if (pos < strArgv.size()) {
            std::cerr << "Trailing characters after number: " << strArgv
                      << std::endl;
        }

        Client client(argv[1], port);
        client.startClient();
    } else {
        // Connecting to a server at the default address
        Client client;
        client.startClient();
    }
}