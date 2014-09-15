/*
Copyright (c) 2014 Argent77

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef FUNCS_H
#define FUNCS_H
#include <cstdint>

namespace tc {

// The following functions may be used for endian-swapping,
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
static inline uint16_t get16u(const uint16_t *ptr, int idx = 0) { return (uint16_t)(((uint8_t*)(&ptr[idx]))[0] << 8 | ((uint8_t*)(&ptr[idx]))[1]); }
static inline int16_t get16s(const int16_t *ptr, int idx = 0)   { return (int16_t)(((uint8_t*)(&ptr[idx]))[0] << 8 | ((uint8_t*)(&ptr[idx]))[1]); }

static inline uint32_t get32u(const uint32_t *ptr, int idx = 0) { return (uint32_t)swap16u((uint16_t*)(&ptr[idx]), 0) << 16 | (uint32_t)swap16u((uint16_t*)(&ptr[idx]), 1); }
static inline int32_t get32s(const int32_t *ptr, int idx = 0)   { return (int32_t)swap16s((int16_t*)(&ptr[idx]), 0) << 16 | (uint32_t)swap16u((uint16_t*)(&ptr[idx]), 1); }

static inline uint64_t get64u(const uint64_t *ptr, int idx = 0) { return (uint64_t)swap32u((uint32_t*)(&ptr[idx]), 0) << 32 | (uint64_t)swap32u((uint32_t*)(&ptr[idx]), 1); }
static inline int64_t get64s(const int64_t *ptr, int idx = 0)   { return (int64_t)swap32s((int32_t*)(&ptr[idx]), 0) << 32 | (uint64_t)swap32u((uint32_t*)(&ptr[idx]), 1); }
#else
static inline uint16_t get16u(const uint16_t *ptr, int idx = 0) { return ptr[idx]; }
static inline int16_t get16s(const int16_t *ptr, int idx = 0) { return ptr[idx]; }

static inline uint32_t get32u(const uint32_t *ptr, int idx = 0) { return ptr[idx]; }
static inline int32_t get32s(const int32_t *ptr, int idx = 0) { return ptr[idx]; }

static inline uint64_t get64u(const uint64_t *ptr, int idx = 0) { return ptr[idx]; }
static inline int64_t get64s(const int64_t *ptr, int idx = 0) { return ptr[idx]; }
#endif


// Performs unconditional byte swaps
static inline uint16_t swap16u(const uint16_t *ptr, int idx = 0) { return (uint16_t)(((uint8_t*)(&ptr[idx]))[0] << 8 | ((uint8_t*)(&ptr[idx]))[1]); }
static inline int16_t swap16s(const int16_t *ptr, int idx = 0)   { return (int16_t)(((uint8_t*)(&ptr[idx]))[0] << 8 | ((uint8_t*)(&ptr[idx]))[1]); }

static inline uint32_t swap32u(const uint32_t *ptr, int idx = 0) { return (uint32_t)swap16u((uint16_t*)(&ptr[idx]), 0) << 16 | (uint32_t)swap16u((uint16_t*)(&ptr[idx]), 1); }
static inline int32_t swap32s(const int32_t *ptr, int idx = 0)   { return (int32_t)swap16s((int16_t*)(&ptr[idx]), 0) << 16 | (uint32_t)swap16u((uint16_t*)(&ptr[idx]), 1); }

static inline uint64_t swap64u(const uint64_t *ptr, int idx = 0) { return (uint64_t)swap32u((uint32_t*)(&ptr[idx]), 0) << 32 | (uint64_t)swap32u((uint32_t*)(&ptr[idx]), 1); }
static inline int64_t swap64s(const int64_t *ptr, int idx = 0)   { return (int64_t)swap32s((int32_t*)(&ptr[idx]), 0) << 32 | (uint64_t)swap32u((uint32_t*)(&ptr[idx]), 1); }

}   // namespace tc

#endif
