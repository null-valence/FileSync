#include "connection.h"
#include "line_parser.h"
#include "protocol.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

Connection::Connection(int socket_fd) : socket_fd_(socket_fd) {}

Connection::~Connection() {
    if(socket_fd_ != -1) {
        close(socket_fd_);
    }
}

bool Connection::send_all(const std::string& message) {
    std::size_t sent =0;
    while(sent < message.size()) {
        ssize_t n = send(socket_fd_, message.data() + sent, message.size() - sent, MSG_NOSIGNAL);
        if(n == -1 && errno == EINTR) continue;
        if(n <= 0) return false;

        sent += static_cast<std::size_t>(n);
    }

    return true;
}

bool Connection::send_message(const std::string& payload) {
    if(payload.size() > UINT32_MAX) return false;
    std::array<std::uint8_t, 4> payload_length = encode_length(payload.size());
    std::string length;
    for(auto& bits : payload_length) length += static_cast<char>(bits);
    
    if (!send_all(length)) return false;
    if (!send_all(payload)) return false;
    return true;
}

bool Connection::receive_line(std::string& message) {
    message.clear();

    while(true) {
        if(extract_line(receive_buffer_, message)) return true;
        
        if(receive_buffer_.size() > 1024) return false;

        char buffer[256];
        ssize_t n = recv(socket_fd_, buffer, sizeof(buffer), 0);
        if(n == -1 && errno == EINTR) continue;
        if(n <= 0) return false;

        receive_buffer_.append(buffer, static_cast<std::size_t>(n));
    }

    return true;
}

bool Connection::receive_exact(char* buffer, std::size_t length) {
    std::size_t received = 0;

    while (received < length) {
        ssize_t n = recv(socket_fd_, buffer + received, length - received, 0);

        if(n == -1 && errno == EINTR) continue;
        if(n <= 0) return false;

        received += static_cast<std::size_t>(n);
    }
    
    return true;
}

bool Connection::receive_message(std::string& payload) {
    char buffer[4];
    if (!receive_exact(buffer, 4)) return false;
    std::array<std::uint8_t, 4> length;
    memcpy(length.data(), buffer, sizeof buffer);
    std::uint32_t payload_length = decode_length(length);
    if(payload_length > MAX_MESSAGE_SIZE) return false;
    payload.clear();
    if (payload_length == 0) return true;

    payload.resize(payload_length);
    if (!receive_exact(payload.data(), payload_length)) return false;

    return true;
}