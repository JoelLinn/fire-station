#include "Sha256.hpp"

#include "RaiiHelpers.hpp"

#include <algorithm>
#include <array>
#include <memory>

#include <openssl/evp.h>

namespace FireStation {
void calculateSha256(const void *data, size_t size, uint8_t out[SHA256_LENGTH]) {
    std::unique_ptr<EVP_MD_CTX, RAII::DeleterFunc<EVP_MD_CTX_free>> ctx(EVP_MD_CTX_new());

    const EVP_MD *md = EVP_get_digestbyname("SHA256");
    if (!md) {
        throw std::runtime_error("Could not find SHA256 digest");
    }

    if (!EVP_DigestInit_ex(ctx.get(), md, NULL)) {
        throw std::runtime_error("Could not initialize SHA256 digest");
    }

    if (!EVP_DigestUpdate(ctx.get(), data, size)) {
        throw std::runtime_error("Could not update SHA256 digest");
    }

    std::array<uint8_t, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestLen = 0;
    if (!EVP_DigestFinal_ex(ctx.get(), digest.data(), &digestLen)) {
        throw std::runtime_error("Could not finalize SHA256 digest");
    }

    std::copy_n(digest.begin(), SHA256_LENGTH, out);
}

std::string sha256ToHex(uint8_t digest[SHA256_LENGTH]) {
    std::string hex(SHA256_LENGTH * 2 + 1, '\0');
    if (OPENSSL_buf2hexstr_ex(hex.data(), hex.size(), nullptr, digest, SHA256_LENGTH, '\0') != 1) {
        throw std::runtime_error("Could not convert SHA256 digest to hex");
    }
    hex.resize(SHA256_LENGTH * 2);
    return hex;
}
} // namespace FireStation
