#include "hasher.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <utility>
#include <openssl/evp.h>

std::optional<std::string> hash_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if(!file.is_open()) {
        std::cerr << "Failed to open file.\n";
        return std::nullopt;
    }

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
        EVP_MD_CTX_new(),
        EVP_MD_CTX_free
    );

    if(ctx == nullptr) {
        return std::nullopt;
    }

    if(EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) == 0) {
        return std::nullopt;
    }
    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    
    while(file) {
        file.read(buffer, buffer_size);
        std::streamsize bytes_read = file.gcount();
        if(EVP_DigestUpdate(ctx.get(), buffer, bytes_read) == 0) {
            return std::nullopt;
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_length;
    if(EVP_DigestFinal_ex(ctx.get(), digest, &digest_length) == 0) {
        return std::nullopt;
    }
    
    std::string result;
    for(unsigned int i = 0; i < digest_length; i++) {
        char character_pair[3];
        snprintf(character_pair, sizeof(character_pair), "%02x", digest[i]);
        result += character_pair;
    }
    return result;
}