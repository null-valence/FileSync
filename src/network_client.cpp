
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

constexpr int PORT = 5000;

bool send_all(int socket_fd, const std::string& message) {
    std::size_t sent = 0;

    while (sent < message.size()) {
        ssize_t n = send(
            socket_fd,
            message.data() + sent,
            message.size() - sent,
            0
        );

        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }

    return true;
}

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

    if (connect(socket_fd,
                reinterpret_cast<sockaddr*>(&server_address),
                sizeof(server_address)) == -1) {
        std::cerr << "connect: " << strerror(errno) << '\n';
        close(socket_fd);
        return 1;
    }

    if (!send_all(socket_fd, "PING\n")) {
        std::cerr << "Failed to send request.\n";
        close(socket_fd);
        return 1;
    }

    std::string response;
    char buffer[256];

    while (response.find('\n') == std::string::npos) {
        ssize_t n = recv(socket_fd, buffer, sizeof(buffer), 0);

        if (n <= 0) {
            std::cerr << "Server disconnected or receive failed.\n";
            close(socket_fd);
            return 1;
        }

        response.append(buffer, static_cast<std::size_t>(n));

        if (response.size() > 1024) {
            std::cerr << "Response too long.\n";
            close(socket_fd);
            return 1;
        }
    }

    if (response != "PONG\n") {
        std::cerr << "Unexpected response received.\n";
        close(socket_fd);
        return 1;
    }

    std::cout << "Server responded: " << response;
    close(socket_fd);
    return 0;
}
