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
#include "compress.h"

namespace tc {

Compression::Compression() noexcept
: m_defResult(false)
, m_infResult(false)
, m_defStream()
, m_infStream()
{
  // initializing compressor with highest compression level
  m_defStream.zalloc = Z_NULL;
  m_defStream.zfree = Z_NULL;
  m_defStream.opaque = Z_NULL;
  m_defResult = (deflateInit(&m_defStream, 9) == Z_OK);

  // initializing decompressor with highest compression level
  m_infStream.zalloc = Z_NULL;
  m_infStream.zfree = Z_NULL;
  m_infStream.opaque = Z_NULL;
  m_infStream.avail_in = 0;
  m_infStream.next_in = Z_NULL;
  m_infResult = (inflateInit(&m_infStream) == Z_OK);
}

Compression::~Compression() noexcept
{
  // cleaning up
  deflateEnd(&m_defStream);
  inflateEnd(&m_infStream);
}

uint32_t Compression::deflate(uint8_t *src, uint32_t srcSize, uint8_t *dst, uint32_t dstSize) noexcept
{
  if (m_defResult && src != nullptr && dst != nullptr && srcSize > 0 && dstSize > 0) {
    m_defStream.avail_in = srcSize;
    m_defStream.next_in = src;
    m_defStream.avail_out = dstSize;
    m_defStream.next_out = dst;
    m_defResult = (::deflate(&m_defStream, Z_FINISH) != Z_STREAM_ERROR);
    uint32_t retVal = dstSize - m_defStream.avail_out;
    deflateReset(&m_defStream);
    return retVal;
  }
  return 0;
}

uint32_t Compression::inflate(uint8_t *src, uint32_t srcSize, uint8_t *dst, uint32_t dstSize) noexcept
{
  if (m_infResult && src != nullptr && dst != nullptr && srcSize > 0&& dstSize > 0) {
    m_infStream.avail_in = srcSize;
    m_infStream.next_in = src;
    m_infStream.avail_out = dstSize;
    m_infStream.next_out = dst;
    m_defResult = (::inflate(&m_infStream, Z_NO_FLUSH) != Z_STREAM_ERROR);
    uint32_t retVal = dstSize - m_infStream.avail_out;
    inflateReset(&m_infStream);
    return retVal;
  }
  return 0;
}

}   // namespace tc
