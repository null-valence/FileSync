#include "connection.h"
#include "line_parser.h"

#include <sys/socket.h>
#include <unistd.h>

Connection::Connection(int socket_fd) : socket_fd_(socket_fd) {}

Connection::~Connection() {
    if(socket_fd_ != -1) {
        close(socket_fd_);
    }
}

bool Connection::send_all(const std::string& message) {
    std:: size_t sent =0;
    while(sent < message.size()) {
        ssize_t n = send(socket_fd_, message.data() + sent, message.size() - sent, 0);
        if(n <= 0) return false;

        sent += static_cast<std::size_t>(n);
    }

    return true;
}

bool Connection::receive_line(std::string& message) {
    message.clear();

    while(true) {
        if(extract_line(receive_buffer_, message)) return true;
        
        if(receive_buffer_.size() > 1024) return false;

        char buffer[256];
        ssize_t n = recv(socket_fd_, buffer, sizeof(buffer), 0);
        if(n <= 0) return false;

        receive_buffer_.append(buffer, static_cast<std::size_t>(n));
    }

    return true;
}