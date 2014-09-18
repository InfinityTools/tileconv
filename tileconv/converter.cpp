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
#include <algorithm>
#include "funcs.h"
#include "converter.h"

namespace tc {

Converter::Converter(const Options& options, unsigned type) noexcept
: m_options(options)
, m_encoding(true)
, m_colorFormat(ColorFormat::ARGB)
, m_type(type)
, m_width()
, m_height()
{
}


Converter::~Converter() noexcept
{
}


void Converter::setWidth(int w) noexcept
{
  m_width = std::max(0, w);
}


void Converter::setHeight(int h) noexcept
{
  m_height = std::max(0, h);
}


int Converter::convert(uint8_t *palette, uint8_t *indexed, uint8_t *encoded) noexcept
{
  return convert(palette, indexed, encoded, 0, 0);
}


unsigned Converter::PadBlock(const uint8_t *src, uint8_t *dst, int srcWidth, int srcHeight,
                             int dstWidth, int dstHeight, bool useCopy) noexcept
{
  if (src != nullptr && dst != nullptr && srcWidth > 0 && srcHeight > 0 &&
      dstWidth >= srcWidth && dstHeight >= srcHeight) {

    for (int y = 0; y < srcHeight; y++) {
      for (int x = 0; x < srcWidth; x++, src += 4, dst += 4) {
        dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
      }
      // padding horizontally with previously used values
      for (int x = srcWidth; x < dstWidth; x++, dst += 4) {
        if (useCopy) {
          dst[0] = dst[-4]; dst[1] = dst[-3]; dst[2] = dst[-2]; dst[3] = dst[-1];
        } else {
          dst[0] = dst[1] = dst[2] = dst[3] = 0;
        }
      }
    }

    // padding vertically with previously used values
    int shift = -(dstWidth * 4);  // offset into previous line
    for (int y = srcHeight; y < dstHeight; y++) {
      for (int x = 0; x < dstWidth; x++, dst += 4) {
        if (useCopy) {
          dst[0] = dst[shift+0]; dst[1] = dst[shift+1]; dst[2] = dst[shift+2]; dst[3] = dst[shift+3];
        } else {
          dst[0] = dst[1] = dst[2] = dst[3] = 0;
        }
      }
    }

    return dstWidth*dstHeight;
  }
  return 0;
}


unsigned Converter::UnpadBlock(const uint8_t *src, uint8_t *dst, int srcWidth, int srcHeight,
                               int dstWidth, int dstHeight) noexcept
{
  if (src != nullptr && dst != nullptr && dstWidth > 0 && dstHeight > 0 &&
      dstWidth <= srcWidth && dstHeight <= srcHeight) {

    int srcStep = (srcWidth-dstWidth)*4;
    for (int y = 0; y < dstHeight; y++, src += srcStep) {
      for (int x = 0; x < dstWidth; x++, src += 4, dst += 4) {
        dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
      }
    }

    return dstWidth*dstHeight;
  }
  return 0;
}


bool Converter::ReorderColors(uint8_t *buffer, unsigned numPixels,
                              ColorFormat from, ColorFormat to) noexcept
{
  if (buffer != nullptr && numPixels >= 0) {
    int c0 = 0, c1 = 0, c2 = 0, c3 = 0;   // defines number of bits to shift for each component
    switch (from) {
      case ColorFormat::ARGB:
        switch (to) {
          case ColorFormat::ABGR: c0 = 16; c2 = -16; break;
          case ColorFormat::BGRA: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          case ColorFormat::RGBA: c0 = 8; c1 = 8; c2 = 8; c3 = -24; break;
          default: return true;
        }
        break;
      case ColorFormat::ABGR:
        switch (to) {
          case ColorFormat::ARGB: c0 = 16; c2 = -16; break;
          case ColorFormat::BGRA: c0 = 8; c1 = 8; c2 = 8; c3 = -24; break;
          case ColorFormat::RGBA: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          default: return true;
        }
        break;
      case ColorFormat::BGRA:
        switch (to) {
          case ColorFormat::ARGB: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          case ColorFormat::ABGR: c0 = 24; c1 = -8; c2 = -8; c3 = -8; break;
          case ColorFormat::RGBA: c1 = 16; c3 = -16; break;
          default: return true;
        }
        break;
      case ColorFormat::RGBA:
        switch (to) {
          case ColorFormat::ARGB: c0 = 24; c1 = -8; c2 = -8; c3 = -8; break;
          case ColorFormat::ABGR: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          case ColorFormat::BGRA: c1 = 16; c3 = -16; break;
          default: return true;
        }
        break;
    }

    uint32_t *src = (uint32_t*)buffer;
    for (unsigned i = 0; i < numPixels; i++, src++) {
      uint32_t srcPixel = get32u_le(src);
      uint32_t dstPixel = 0;
      dstPixel |= (srcPixel & 0x000000ff) << c0;
      dstPixel |= (c1 < 0) ? (srcPixel & 0x0000ff00) >> c1 : (srcPixel & 0x0000ff00) << c1;
      dstPixel |= (c2 < 0) ? (srcPixel & 0x00ff0000) >> c2 : (srcPixel & 0x00ff0000) << c2;
      dstPixel |= (srcPixel & 0xff000000) >> c3;
      *src = get32u_le(&dstPixel);
    }

    return true;
  }
  return false;
}

}   // namespace tc
