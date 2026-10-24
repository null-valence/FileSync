
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "connection.h"

constexpr int PORT = 5000;

int main() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        std::cerr << "socket: " << strerror(errno) << '\n';
        return 1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(socket_fd, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) == -1) {
        std::cerr << "connect: " << strerror(errno) << '\n';
        close(socket_fd);
        return 1;
    }

    Connection connection(socket_fd);

    if (!connection.send_all("PING\n")) {
        std::cerr << "Failed to send request.\n";
        return 1;
    }

    std::string response;

    if (!connection.receive_line(response)) {
        std::cerr << "Failed to receive response.\n";
        return 1;
    }

    if (response != "PONG\n") {
        std::cerr << "Unexpected response received.\n";
        return 1;
    }

    std::cout << "Server responded: " << response;
    return 0;
}
