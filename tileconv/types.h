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
#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <memory>

namespace tc {

/**
 * Supported input file types:
 * UNKNOWN: Returned in case of error
 * TIS:     Paletted TIS resource type
 * MOS:     Paletted MOS resource type
 * TBC:     Compressed TIS resource type
 * MBC:     Compressed MOS resource type
 */
enum class FileType { UNKNOWN, TIS, MOS, TBC, MBC };

/**
 * Supported pixel compression types:
 * UNKNOWN: Returned in case of error
 * RAW:     Original palette+indexed pixel data
 * BC1:     Using DXT1 compression
 * BC2:     Using DXT3 compression
 * BC3:     Using DXT5 compression
 */
enum class Encoding { UNKNOWN, RAW, BC1, BC2, BC3 };

typedef std::shared_ptr<uint8_t> BytePtr;

static const unsigned HEADER_TBC_SIZE             = 16;       // TBC header size
static const unsigned HEADER_MBC_SIZE             = 20;       // MBC header size
static const unsigned HEADER_TILE_ENCODED_SIZE    = 4;        // header size for a raw/BCx encoded tile
static const unsigned HEADER_TILE_COMPRESSED_SIZE = 4;        // header size for a zlib compressed tile

static const unsigned PALETTE_SIZE                = 1024;     // palette size in bytes
static const unsigned MAX_TILE_SIZE_8             = 64*64;    // max. size (in bytes) of a 8-bit pixels tile
static const unsigned MAX_TILE_SIZE_32            = 64*64*4;  // max. size (in bytes) of a 32-bit pixels tile

}   // namespace tc

#endif
