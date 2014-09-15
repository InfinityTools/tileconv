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
#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstdint>
#include <zlib.h>

namespace tc {

/** Provides zlib compression and decompression routines. */
class Compression
{
public:
  Compression() noexcept;
  ~Compression() noexcept;

  /**
   * Applies zlib compression to a source data block and stores the result in a target data block.
   * \param src The source data block with raw data.
   * \param srcSize The number of bytes to compress.
   * \param dst  The data block to store the compressed data into.
   * \param dstSize The number of bytes available to store data into.
   * \return The size of the compressed data stored in the target data block or 0 on error.
   */
  uint32_t deflate(uint8_t *src, uint32_t srcSize, uint8_t *dst, uint32_t dstSize) noexcept;

  /**
   * Applies zlib decompression to a source data block and stores the result in a target data block.
   * \param src The source data containing the compressed data including size prefix.
   * \param srcSize The number of bytes to decompress.
   * \param dst The target block to store the decompressed data into.
   * \param dstSize The number of bytes available to store data into.
   * \return The size of the decompressed data stored in the target data block or 0 on error.
   */
  uint32_t inflate(uint8_t *src, uint32_t srcSize, uint8_t *dst, uint32_t dstSize) noexcept;

private:
  bool      m_defResult;    // stores the current state of the compressor
  bool      m_infResult;    // stores the current state of the decompressor
  z_stream  m_defStream;    // working structure for deflate
  z_stream  m_infStream;    // working structure for inflate
};

}   // namespace tc

#endif
