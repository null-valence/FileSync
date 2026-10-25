#include <gtest/gtest.h>
#include "connection.h"
#include "line_parser.h"
#include "protocol.h"
#include <sys/socket.h>

// Test 1
TEST(ConnectionTest, PreservesMultipleMessages) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    ASSERT_TRUE(client.send_all("PING\nHello\nWorld!\n"));
    std::string message;
    ASSERT_TRUE(server.receive_line(message));
    ASSERT_EQ(message, "PING\n");
    ASSERT_TRUE(server.receive_line(message));
    ASSERT_EQ(message, "Hello\n");
    ASSERT_TRUE(server.receive_line(message));
    ASSERT_EQ(message, "World!\n");
}

// Test 2
TEST(LineParserTest, ExtractsMultipleMessages) {
    std::string buffer = "PING\nHello\nWorld!\n";
    std::string message;

    ASSERT_TRUE(extract_line(buffer, message));
    EXPECT_EQ(message, "PING\n");
    EXPECT_EQ(buffer, "Hello\nWorld!\n");

    ASSERT_TRUE(extract_line(buffer, message));
    EXPECT_EQ(message, "Hello\n");
    EXPECT_EQ(buffer, "World!\n");

    ASSERT_TRUE(extract_line(buffer, message));
    EXPECT_EQ(message, "World!\n");
    EXPECT_TRUE(buffer.empty());
}

// Test 3
TEST(LineParserTest, ReturnsFalseWhenNoCompleteLineExists) {
    std::string buffer = "PING";
    std::string message;

    EXPECT_FALSE(extract_line(buffer, message));
    EXPECT_EQ(buffer, "PING");
}

// Test 4
TEST(ConnectionTest, DisconnectedPeer) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);
    
    Connection client(socket_fds[1]);
    close(socket_fds[0]);

    ASSERT_FALSE(client.send_all("PING\n"));
}

// Test 5
TEST(ConnectionTest, ReceiveFailsWhenPeerDisconnects) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);
    
    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);
    
    ASSERT_TRUE(client.send_all("PING"));
    ASSERT_EQ(shutdown(socket_fds[1], SHUT_WR), 0);
    
    std::string message;
    ASSERT_FALSE(server.receive_line(message));
}

// Test 6
TEST(ConnectionTest, ReceiveExactReadsRequestedBytes) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    ASSERT_TRUE(client.send_all("Hello"));
    
    char buffer[5];
    ASSERT_TRUE(server.receive_exact(buffer, 5));
    ASSERT_EQ(std::string(buffer, sizeof(buffer)), "Hello");
}

// Test 7
TEST(ConnectionTest, ReceiveExactReadsLessBytes) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    ASSERT_TRUE(client.send_all("Hel"));
    ASSERT_EQ(shutdown(socket_fds[1], SHUT_WR), 0);
    
    char buffer[5];
    ASSERT_FALSE(server.receive_exact(buffer, 5));
}

// Test 8
TEST(ConnectionTest, ReceiveExactReadsNullBytes) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    std::string message("A\0B\nC", 5);
    ASSERT_TRUE(client.send_all(message));
    
    char buffer[5];
    ASSERT_TRUE(server.receive_exact(buffer, 5));
    ASSERT_EQ(std::string(buffer, sizeof(buffer)), message);
}

// Test 9
TEST(ProtocolTest, EncodesLengthInNetworkByteOrder) {
    auto length = encode_length(5);
    ASSERT_EQ(length.size(), 4);
    ASSERT_EQ(length[0], 0);
    ASSERT_EQ(length[1], 0);
    ASSERT_EQ(length[2], 0);
    ASSERT_EQ(length[3], 5);
}

// Test 10
TEST(ProtocolTest, DecodesLengthFromNetworkByteOrder) {
    auto result = decode_length(std::array <uint8_t, 4> {0, 0, 0, 5});
    ASSERT_EQ(result, 5);
}

// Test 11
TEST(ProtocolTest, EncodeDecodeRoundTrip) {
    auto length = encode_length(123456);
    auto result = decode_length(length);
    ASSERT_EQ(result, 123456);
}

// Test 12
TEST(ConnectionTest, SendsAndReceivesLengthPrefixedMessage) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    ASSERT_TRUE(client.send_message("Hello"));
    
    std::string message;
    ASSERT_TRUE(server.receive_message(message));
    ASSERT_EQ(message, "Hello");
}

// Test 13
TEST(ConnectionTest, SendsAndReceivesLengthPrefixedEmptyMessage) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    ASSERT_TRUE(client.send_message(""));
    
    std::string message;
    ASSERT_TRUE(server.receive_message(message));
    ASSERT_TRUE(message.empty());
}

// Test 14
TEST(ConnectionTest, SendsAndReceivesLengthPrefixedEmptySizeMessage) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    std::string message = "Old data";
    std::string empty_length("\0\0\0\0", 4);
    ASSERT_TRUE(client.send_all(empty_length + message));
    
    ASSERT_TRUE(server.receive_message(message));
    ASSERT_TRUE(message.empty());
}

// Test 15
TEST(ConnectionTest, RejectsOversizedMessage) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    auto encoded = encode_length(MAX_MESSAGE_SIZE + 1);
    std::string oversized_length;
    for (auto byte : encoded) {
        oversized_length += static_cast<char>(byte);
    }
    
    ASSERT_TRUE(client.send_all(oversized_length));
    std::string message = "Old data";
    ASSERT_FALSE(server.receive_message(message));
}

// Test 16
TEST(ConnectionTest, TransfersBinaryPayload) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    std::string payload("A\0B\nC", 5);
    ASSERT_TRUE(client.send_message(payload));
    
    std::string message;
    ASSERT_TRUE(server.receive_message(message));
    ASSERT_EQ(message, payload);
}