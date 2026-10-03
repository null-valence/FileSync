#include "hasher.h"

#include <iostream>
#include <fstream>
#include <openssl/evp.h>

optional<string> hash_file(const fs::path& path) {
    ifstream file(path, ios::binary);
    if(!file.is_open()) {
        cerr << "Failed to open file.\n";
        return nullptr;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    
    while(file) {
        file.read(buffer, buffer_size);
        streamsize bytes_read = file.gcount();
        EVP_DigestUpdate(ctx, buffer, bytes_read);
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_length;
    EVP_DigestFinal_ex(ctx, digest, &digest_length);
    EVP_MD_CTX_free(ctx);
    
    string result;
    for(unsigned int i = 0; i < digest_length; i++) {
        char character_pair[3];
        snprintf(character_pair, sizeof(character_pair), "%02x", digest[i]);
        result += character_pair;
    }
    return result;
}