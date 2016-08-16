#pragma once

#include "Base64.h"
#include "Decompress.h"
#include <vector>
#include <cassert>

inline std::vector<double> DecodeMzData(const char* encoded_data, size_t size, int precision, bool is_zlib, bool little_endian) {
    typedef char byte;

    // base64 decode
    auto buffer_size = size / 4 * 3;  // enough for decoding base64
    byte* buffer = new byte[buffer_size + 1]();  // doing initialization at the same time
    auto decoded_size = decode(encoded_data, size, buffer);

    // zlib decompress
    unsigned long dest_len;
    byte* dest = nullptr;
    if (is_zlib) {
        dest = decompress(buffer, decoded_size, &dest_len);
        delete[] buffer;
    }
    else {
        dest = buffer;
        dest_len = decoded_size;
    }

    // retrieve values
    assert(precision == 64 || precision == 32);  // only these two precisions are allowed.
    auto step_size = precision / 8;  // step size in byte unit
    auto vector_size = dest_len / step_size;
    std::vector<double> decoded_data(vector_size);  // pre-allocate space
    if (little_endian) {
        if (precision == 64) {
            for (unsigned i = 0; i < vector_size; ++i) {
                double dbuf;
                byte* ptr = reinterpret_cast<byte*>(&dbuf);
                for (auto j = 0; j < step_size; ++j) {
                    *(ptr + j) = dest[i * step_size + j];
                }
                decoded_data[i] = dbuf;
            }
        }
        else {  // 32 bit
            for (unsigned i = 0; i < vector_size; ++i) {
                float fbuf;
                byte* ptr = reinterpret_cast<byte*>(&fbuf);
                for (auto j = 0; j < step_size; ++j) {
                    *(ptr + j) = dest[i * step_size + j];
                }
                decoded_data[i] = fbuf;
            }
        }
    }
    else {  // big endian
        if (precision == 64) {
            for (unsigned i = 0; i < vector_size; ++i) {
                double dbuf;
                byte* ptr = reinterpret_cast<byte*>(&dbuf);
                for (auto j = 0; j < step_size; ++j) {
                    *(ptr + j) = dest[i * step_size + step_size - 1 - j];
                }
                decoded_data[i] = dbuf;
            }
        }
        else {  // 32 bit
            for (unsigned i = 0; i < vector_size; ++i) {
                float fbuf;
                byte* ptr = reinterpret_cast<byte*>(&fbuf);
                for (auto j = 0; j < step_size; ++j) {
                    *(ptr + j) = dest[i * step_size + step_size - 1 - j];
                }
                decoded_data[i] = fbuf;
            }
        }
    }
    delete[] dest;
    return decoded_data;
}
