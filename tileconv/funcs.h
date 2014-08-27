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

// The following functions may be used for endian-swapping,
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
static inline uint16_t get16u(const uint16_t *ptr, int idx = 0) { return static_cast<uint16_t>(reinterpret_cast<const uint8_t*>(&ptr[idx])[0]) << 8 | reinterpret_cast<const uint8_t*>(&ptr[idx])[1]; }
static inline int16_t get16s(const int16_t *ptr, int idx = 0) { return static_cast<int16_t>(reinterpret_cast<const uint8_t*>(&ptr[idx])[0]) << 8 | reinterpret_cast<const uint8_t*>(&ptr[idx])[1]; }
static inline uint32_t get32u(const uint32_t *ptr, int idx = 0) { return (static_cast<uint32_t>(get16u(reinterpret_cast<const uint16_t*>(&ptr[idx]), 0)) << 16) | get16u(reinterpret_cast<const uint16_t*>(&ptr[idx]), 1); }
static inline int32_t get32s(const int32_t *ptr, int idx = 0) { return (static_cast<int32_t>(get16s(reinterpret_cast<const int16_t*>(&ptr[idx]), 0)) << 16) | get16u(reinterpret_cast<const uint16_t*>(&ptr[idx]), 1); }
#else
static inline uint16_t get16u(const uint16_t *ptr, int idx = 0) { return ptr[idx]; }
static inline int16_t get16s(const int16_t *ptr, int idx = 0) { return ptr[idx]; }
static inline uint32_t get32u(const uint32_t *ptr, int idx = 0) { return ptr[idx]; }
static inline int32_t get32s(const int32_t *ptr, int idx = 0) { return ptr[idx]; }
#endif


#endif
