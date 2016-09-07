#pragma once

#include <zlib.h>
#include <stdexcept>

// a customized wrapper for decompression, handling unknown uncompressed data size
inline char* decompress(const char* source, unsigned long source_len, unsigned long* dest_len) {
    auto buffer_size = 8192;  // if not enough, double the buffer size.
    auto buffer = new unsigned char[buffer_size]();  // dest buffer
    *dest_len = buffer_size;
    auto state = uncompress(buffer, dest_len, reinterpret_cast<const unsigned char*>(source), source_len);
    while (state != Z_OK) {
        switch (state) {
        case Z_MEM_ERROR:
            throw std::runtime_error("No enough memory for decompression.");
        case Z_DATA_ERROR:
            throw std::runtime_error("Compressed data is broken.");
        case Z_BUF_ERROR:
            // buffer is not enough, so double the buffer size.
            delete[] buffer;
            buffer_size *= 2;
            buffer = new unsigned char[buffer_size]();
            *dest_len = buffer_size;
            state = uncompress(buffer, dest_len, reinterpret_cast<const unsigned char*>(source), source_len);
            break;
        default:
            throw std::runtime_error("Impossible path in decompressing.");
        }
    }
    return reinterpret_cast<char*>(buffer);
}
