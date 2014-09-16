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
#include "jpeg.h"

namespace tc {

Jpeg::Jpeg() noexcept
: m_handle()
{
  updateError((m_handle = tjInitDecompress()) == nullptr);
}

Jpeg::~Jpeg() noexcept
{
  if (m_handle != nullptr) {
    tjDestroy(m_handle);
    m_handle = nullptr;
  }
}


bool Jpeg::decompress(uint8_t *srcBuf, size_t srcBufSize, uint8_t *dstBuf,
                      PixelFormat pf, int flags) noexcept
{
  bool retVal = false;
  if (m_handle != nullptr && srcBuf != nullptr && srcBufSize > 0 && dstBuf != nullptr) {
    retVal = (tjDecompress2(m_handle, srcBuf, srcBufSize, dstBuf, 0, 0, 0, pf, flags) == 0);
    updateError(!retVal);
  }
  return retVal;
}


bool Jpeg::updateInformation(uint8_t *buf, size_t bufSize) noexcept
{
  bool retVal = false;
  if (m_handle != nullptr && buf != nullptr && bufSize > 0) {
    retVal = (tjDecompressHeader2(m_handle, buf, bufSize, &m_width, &m_height, &m_subSampling) == 0);
    updateError(!retVal);
  }
  return retVal;
}


std::string Jpeg::getErrorMsg() const noexcept
{
  if (isError()) {
    return std::string(tjGetErrorStr());
  } else {
    return std::string("");
  }
}

}   // namespace tc
