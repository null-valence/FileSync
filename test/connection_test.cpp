#include <gtest/gtest.h>
#include "connection.h"
#include "protocol.h"
#include "file_transfer.h"

#include <sys/socket.h>
#include <thread>
#include <fstream>

// Test 1
TEST(ConnectionTest, DisconnectedPeer) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);
    
    Connection client(socket_fds[1]);
    close(socket_fds[0]);

    ASSERT_FALSE(client.send_all("PING\n"));
}

// Test 2
TEST(ConnectionTest, ReceiveFailsWhenPeerDisconnects) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);
    
    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);
    
    ASSERT_EQ(shutdown(socket_fds[1], SHUT_WR), 0);    
    std::string message;
    ASSERT_FALSE(server.receive_message(message));
}

// Test 3
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

// Test 4
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

// Test 5
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

// Test 6
TEST(ProtocolTest, EncodesLengthInNetworkByteOrder) {
    auto length = encode_length(5);
    ASSERT_EQ(length.size(), 4);
    ASSERT_EQ(length[0], 0);
    ASSERT_EQ(length[1], 0);
    ASSERT_EQ(length[2], 0);
    ASSERT_EQ(length[3], 5);
}

// Test 7
TEST(ProtocolTest, DecodesLengthFromNetworkByteOrder) {
    auto result = decode_length(std::array <uint8_t, 4> {0, 0, 0, 5});
    ASSERT_EQ(result, 5);
}

// Test 8
TEST(ProtocolTest, EncodeDecodeRoundTrip) {
    auto length = encode_length(123456);
    auto result = decode_length(length);
    ASSERT_EQ(result, 123456);
}

// Test 9
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

// Test 10
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

// Test 11
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

// Test 12
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

// Test 13
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

// Test 14
TEST(FileTransferTest, TransferSmallFile) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);
    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootA / "file.txt") << "Hello";
    }

    fs::path source = rootA / "file.txt";
    fs::path destination = rootB / "file.txt";

    bool send_result = false;
    bool receive_result = false;

    std::thread sender([&]() {
        send_result = transfer_file(source, client);
    });

    std::thread receiver([&]() {
        receive_result = receive_file(destination, server);
    });

    sender.join();
    receiver.join();

    ASSERT_TRUE(send_result);
    ASSERT_TRUE(receive_result);

    ASSERT_TRUE(fs::exists(destination));
    std::ifstream in(destination);
    std::string content;
    std::getline(in, content);

    ASSERT_EQ(content, "Hello");
}

// Test 15
TEST(FileTransferTest, TransferEmptyFile) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);
    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootA / "file.txt");
    }

    fs::path source = rootA / "file.txt";
    fs::path destination = rootB / "file.txt";

    bool send_result = false;
    bool receive_result = false;

    std::thread sender([&]() {
        send_result = transfer_file(source, client);
    });

    std::thread receiver([&]() {
        receive_result = receive_file(destination, server);
    });

    sender.join();
    receiver.join();

    ASSERT_TRUE(send_result);
    ASSERT_TRUE(receive_result);

    ASSERT_TRUE(fs::exists(destination));
    ASSERT_TRUE(fs::is_empty(destination));
}

// Test 16
TEST(FileTransferTest, TransferLargeFile) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);
    fs::create_directories(rootA);
    fs::create_directories(rootB);

    fs::path source = rootA / "file.txt";
    fs::path destination = rootB / "file.txt";
    {
        std::ofstream(source) << "Hello World!\n";
        fs::resize_file(source, 5000);
    }

    bool send_result = false;
    bool receive_result = false;

    std::thread sender([&]() {
        send_result = transfer_file(source, client);
    });

    std::thread receiver([&]() {
        receive_result = receive_file(destination, server);
    });

    sender.join();
    receiver.join();

    ASSERT_TRUE(send_result);
    ASSERT_TRUE(receive_result);

    ASSERT_TRUE(fs::exists(destination));
    ASSERT_EQ(fs::file_size(destination), fs::file_size(source));

    std::ifstream in_source(source, std::ios_base::binary), in_destination(destination, std::ios_base::binary);
    char in_A, in_B;

    while (in_source.get(in_A)) {
        ASSERT_TRUE(in_destination.get(in_B));
        ASSERT_EQ(in_A, in_B);
    }

    ASSERT_TRUE(in_source.eof());
    ASSERT_FALSE(in_destination.get(in_B));
    ASSERT_TRUE(in_destination.eof());
}

// Test 17
TEST(FileTransferTest, TransferBinaryFile) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);
    fs::create_directories(rootA);
    fs::create_directories(rootB);

    fs::path source = rootA / "file.txt";
    fs::path destination = rootB / "file.txt";
    {
        std::ofstream(source) << std::string("H\nello\0 W\no\0rld!\n", 17);
    }

    bool send_result = false;
    bool receive_result = false;

    std::thread sender([&]() {
        send_result = transfer_file(source, client);
    });

    std::thread receiver([&]() {
        receive_result = receive_file(destination, server);
    });

    sender.join();
    receiver.join();

    ASSERT_TRUE(send_result);
    ASSERT_TRUE(receive_result);

    ASSERT_TRUE(fs::exists(destination));
    ASSERT_EQ(fs::file_size(destination), fs::file_size(source));

    std::ifstream in_source(source, std::ios_base::binary), in_destination(destination, std::ios_base::binary);
    char in_A, in_B;

    while (in_source.get(in_A)) {
        ASSERT_TRUE(in_destination.get(in_B));
        ASSERT_EQ(in_A, in_B);
    }

    ASSERT_TRUE(in_source.eof());
    ASSERT_FALSE(in_destination.get(in_B));
    ASSERT_TRUE(in_destination.eof());
}

// Test 18
TEST(FileTransferTest, TransferVeryLargeFile) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);
    fs::create_directories(rootA);
    fs::create_directories(rootB);

    fs::path source = rootA / "file.txt";
    fs::path destination = rootB / "file.txt";
    {
        std::ofstream(source) << "Hello World!\n";
        fs::resize_file(source, MAX_MESSAGE_SIZE+10);
    }

    bool send_result = false;
    bool receive_result = false;

    std::thread sender([&]() {
        send_result = transfer_file(source, client);
    });

    std::thread receiver([&]() {
        receive_result = receive_file(destination, server);
    });

    sender.join();
    receiver.join();

    ASSERT_TRUE(send_result);
    ASSERT_TRUE(receive_result);

    ASSERT_TRUE(fs::exists(destination));
    ASSERT_EQ(fs::file_size(destination), fs::file_size(source));

    std::ifstream in_source(source, std::ios_base::binary), in_destination(destination, std::ios_base::binary);
    char in_A, in_B;

    while (in_source.get(in_A)) {
        ASSERT_TRUE(in_destination.get(in_B));
        ASSERT_EQ(in_A, in_B);
    }

    ASSERT_TRUE(in_source.eof());
    ASSERT_FALSE(in_destination.get(in_B));
    ASSERT_TRUE(in_destination.eof());
}

// Test 19
TEST(FileTransferTest, RejectOversizedFile) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);
    fs::create_directories(rootA);
    fs::create_directories(rootB);

    fs::path destination = rootB / "file.txt";

    ASSERT_TRUE(client.send_message(std::to_string(MAX_FILE_SIZE+1)));
    ASSERT_FALSE(receive_file(destination, server));
    ASSERT_FALSE(fs::exists(destination));
}

// Test 20
TEST(FileTransferTest, RejectTruncatedTransfer) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);
    fs::create_directories(rootA);
    fs::create_directories(rootB);
    
    fs::path source = rootA / "file.txt";
    fs::path destination = rootB / "file.txt";

    {
        std::ofstream(source) << "Hello";
    }

    bool send_result = false;
    bool receive_result = false;

    std::thread sender([&]() {
        send_result = client.send_message(std::to_string(100));
        if (!client.send_message("Hello")) send_result = false;
        if(shutdown(socket_fds[1], SHUT_WR) != 0) send_result = 0;
    });

    std::thread receiver([&]() {
        receive_result = receive_file(destination, server);
    });

    sender.join();
    receiver.join();

    ASSERT_TRUE(send_result);
    ASSERT_FALSE(receive_result);

    ASSERT_FALSE(fs::exists(destination));
}

// Test 21
TEST(FileTransferTest, PreserveExistingDestinationOnTruncatedTransfer) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);
    fs::create_directories(rootA);
    fs::create_directories(rootB);
    
    fs::path destination = rootB / "file.txt";

    {
        std::ofstream(destination) << "Original content";
    }

    bool send_result = false;
    bool receive_result = false;

    std::thread sender([&]() {
        send_result = client.send_message(std::to_string(100));
        if (!client.send_message("Hello")) send_result = false;
        if(shutdown(socket_fds[1], SHUT_WR) != 0) send_result = 0;
    });

    std::thread receiver([&]() {
        receive_result = receive_file(destination, server);
    });

    sender.join();
    receiver.join();

    ASSERT_TRUE(send_result);
    ASSERT_FALSE(receive_result);

    ASSERT_TRUE(fs::exists(destination));
    ASSERT_FALSE(fs::exists(rootB / (".filesync-" + destination.filename().string() + ".tmp")));
    std::ifstream in(destination);
    std::string content;
    std::getline(in, content);

    ASSERT_EQ(content, "Original content");
}

// Test 22
TEST(FileTransferTest, RejectEmptyFilesizeMetadata) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path root = "/tmp/filesync-test";
    fs::remove_all(root);
    fs::create_directories(root);

    fs::path destination = root / "file.txt";

    ASSERT_TRUE(client.send_message(""));

    ASSERT_FALSE(receive_file(destination, server));
    ASSERT_FALSE(fs::exists(destination));
    ASSERT_FALSE(fs::exists(root / ".filesync-file.txt.tmp"));
}

// Test 23
TEST(FileTransferTest, RejectNonNumericMetadata) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path root = "/tmp/filesync-test";
    fs::remove_all(root);
    fs::create_directories(root);

    fs::path destination = root / "file.txt";

    ASSERT_TRUE(client.send_message("abc"));

    ASSERT_FALSE(receive_file(destination, server));
    ASSERT_FALSE(fs::exists(destination));
    ASSERT_FALSE(fs::exists(root / ".filesync-file.txt.tmp"));
}

// Test 24
TEST(FileTransferTest, RejectTrailingCharactersMetadata) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path root = "/tmp/filesync-test";
    fs::remove_all(root);
    fs::create_directories(root);

    fs::path destination = root / "file.txt";

    ASSERT_TRUE(client.send_message("100xyz"));

    ASSERT_FALSE(receive_file(destination, server));
    ASSERT_FALSE(fs::exists(destination));
    ASSERT_FALSE(fs::exists(root / ".filesync-file.txt.tmp"));
}

// Test 25
TEST(FileTransferTest, RejectNegativeSizeMetadata) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path root = "/tmp/filesync-test";
    fs::remove_all(root);
    fs::create_directories(root);

    fs::path destination = root / "file.txt";

    ASSERT_TRUE(client.send_message("-1"));

    ASSERT_FALSE(receive_file(destination, server));
    ASSERT_FALSE(fs::exists(destination));
    ASSERT_FALSE(fs::exists(root / ".filesync-file.txt.tmp"));
}

// Test 26
TEST(FileTransferTest, RejectTooLargeSizeValueMetadata) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    const fs::path root = "/tmp/filesync-test";
    fs::remove_all(root);
    fs::create_directories(root);

    fs::path destination = root / "file.txt";

    ASSERT_TRUE(client.send_message("184467440737095516160"));

    ASSERT_FALSE(receive_file(destination, server));
    ASSERT_FALSE(fs::exists(destination));
    ASSERT_FALSE(fs::exists(root / ".filesync-file.txt.tmp"));
}