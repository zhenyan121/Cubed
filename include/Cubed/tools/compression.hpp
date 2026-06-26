#pragma once
#include <cstdint>
#include <format>
#include <span>
#include <stdexcept>
#include <vector>
#include <zstd.h>
namespace Cubed {
constexpr int DEFAULT_ZSTD_LEVEL = 3;
inline std::vector<uint8_t> compress_data(std::span<const uint8_t> data) {
    size_t max_size = ZSTD_compressBound(data.size());
    std::vector<uint8_t> compressed_data(max_size);
    size_t compressed_bytes =
        ZSTD_compress(compressed_data.data(), max_size, data.data(),
                      data.size(), DEFAULT_ZSTD_LEVEL);
    if (ZSTD_isError(compressed_bytes)) {
        throw std::runtime_error(std::format(
            "Compress Fail {}", ZSTD_getErrorName(compressed_bytes)));
    }
    compressed_data.resize(compressed_bytes);
    return compressed_data;
}

inline std::vector<uint8_t> decompress_data(std::span<const uint8_t> data,
                                            uint32_t original_size) {
    std::vector<uint8_t> decompressed_data(original_size);
    size_t decompressed_bytes = ZSTD_decompress(
        decompressed_data.data(), original_size, data.data(), data.size());
    if (ZSTD_isError(decompressed_bytes)) {
        throw std::runtime_error(std::format(
            "Decompress Fail {}", ZSTD_getErrorName(decompressed_bytes)));
    }
    if (decompressed_bytes != original_size) {
        throw std::runtime_error("Unexpected decompressed size");
    }

    return decompressed_data;
}

} // namespace Cubed
