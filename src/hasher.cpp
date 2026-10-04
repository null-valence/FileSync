#include "hasher.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <utility>
#include <openssl/evp.h>

optional<string> hash_file(const fs::path& path) {
    ifstream file(path, ios::binary);
    if(!file.is_open()) {
        cerr << "Failed to open file.\n";
        return nullopt;
    }

    unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
        EVP_MD_CTX_new(),
        EVP_MD_CTX_free
    );

    if(ctx == nullptr) {
        return nullopt;
    }

    if(EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) == 0) {
        return nullopt;
    }
    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    
    while(file) {
        file.read(buffer, buffer_size);
        streamsize bytes_read = file.gcount();
        if(EVP_DigestUpdate(ctx.get(), buffer, bytes_read) == 0) {
            return nullopt;
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_length;
    if(EVP_DigestFinal_ex(ctx.get(), digest, &digest_length) == 0) {
        return nullopt;
    }
    
    string result;
    for(unsigned int i = 0; i < digest_length; i++) {
        char character_pair[3];
        snprintf(character_pair, sizeof(character_pair), "%02x", digest[i]);
        result += character_pair;
    }
    return result;
}