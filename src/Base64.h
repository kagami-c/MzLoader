#pragma once

extern "C" {
#include <b64/cdecode.h>
}

// a customized wrapper of libb64 library
inline int decode(const char* code_in, const int length_in, char* plaintext_out) {
    base64_decodestate state;
    base64_init_decodestate(&state);
    return base64_decode_block(code_in, length_in, plaintext_out, &state);
}