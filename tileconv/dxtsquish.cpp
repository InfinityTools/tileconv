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
#include <cstring>
#include "dxtsquish.h"

const int DxtSquish::DEF_FLAGS = Dxt1 | ColourClusterFit | WeightColourByAlpha;
const int DxtSquish::MASK_DXT = Dxt1 | Dxt3 | Dxt5;


DxtSquish::DxtSquish() noexcept
: DxtBase()
{
  setFlags(DEF_FLAGS);
}


DxtSquish::DxtSquish(ColorFormat format) noexcept
: DxtBase(format, DEF_FLAGS)
{
}


DxtSquish::DxtSquish(ColorFormat format, int flags) noexcept
: DxtBase(format, flags)
{
}


DxtSquish::~DxtSquish() noexcept
{
}


bool DxtSquish::compressBlock(uint8_t *src, uint8_t *dst) noexcept
{
  uint8_t buffer[64]; // temp. buffer for a single 4x4 pixel block

  if (src != nullptr && dst != nullptr) {
    std::memcpy(buffer, src, 64);
    ReorderColors(buffer, 16, getColorFormat(), ColorFormat::ABGR);
    squish::Compress(buffer, dst, getFlags());
    return true;
  }
  return false;
}


/*
bool DxtSquish::decompressBlock(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    squish::Decompress(dst, src, getFlags());
    DxtBase::ReorderColors(dst, 16, ColorFormat::ABGR, getColorFormat());
    return true;
  }
  return false;
}
*/


void DxtSquish::setDxt1() noexcept
{
  setFlags((getFlags() & ~MASK_DXT) | Dxt1);
}


void DxtSquish::setDxt3() noexcept
{
  setFlags((getFlags() & ~MASK_DXT) | Dxt3);
}


void DxtSquish::setDxt5() noexcept
{
  setFlags((getFlags() & ~MASK_DXT) | Dxt5);
}


bool DxtSquish::isDxt1() const noexcept
{
  return (getFlags() & Dxt1) != 0;
}


bool DxtSquish::isDxt3() const noexcept
{
  return (getFlags() & Dxt3) != 0;
}


bool DxtSquish::isDxt5() const noexcept
{
  return (getFlags() & Dxt5) != 0;
}

