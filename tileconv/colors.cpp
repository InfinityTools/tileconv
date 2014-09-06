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
#include <limits>
#include <cmath>
#include <cstring>
#include <memory>
#include "colors.h"
#include "colorquant.h"
#include "funcs.h"

Colors::Colors(const Options &options) noexcept
: m_options(options)
{
}

Colors::~Colors() noexcept
{
}

uint32_t Colors::palToARGB(uint8_t *src, uint8_t *palette, uint8_t *dst, uint32_t size) noexcept
{
  if (src != nullptr && palette != nullptr && dst != nullptr && size > 0) {
    for (uint32_t i = 0; i < size; i++, src++, dst += 4) {
      uint32_t ofs = (uint32_t)src[0] << 2;
      if (src[0] || get32u((uint32_t*)palette) != 0x0000ff00) {
        dst[0] = palette[ofs+0];
        dst[1] = palette[ofs+1];
        dst[2] = palette[ofs+2];
        dst[3] = 255;
      } else {
        dst[0] = dst[1] = dst[2] = dst[3] = 0;
      }
    }
    return size;
  }
  return 0;
}

uint32_t Colors::ARGBToPal(uint8_t *src, uint8_t *dst, uint8_t *palette,
                           uint32_t width, uint32_t height) noexcept
{
  if (src != nullptr && dst != nullptr && palette != nullptr && width > 0 && height > 0) {
    uint32_t size = width*height;

    // preparing source pixels
    reorderColors(src, size, FMT_ARGB, FMT_ABGR);

    ColorQuant quant;
    if (!quant.setSource(src, width, height)) return 0;
    if (!quant.setTarget(dst, size)) return 0;
    std::memset(palette, 0, 1024);
    if (!quant.setPalette(palette, 1024)) return 0;
    quant.setSpeed(10 - getOptions().getQuality());   // speed is defined as "10 - quality"

    if (!quant.quantize()) return 0;
    if (getOptions().isVerbose()) {
      double qerr = quant.getQuantizationError();
      if (qerr >= 0.0) {
        if (qerr <= 5.0) {
          std::printf("Color quantization applied. Error ratio: %.2f (excellent!)\n", qerr);
        } else if (qerr <= 10.0) {
          std::printf("Color quantization applied. Error ratio: %.2f (good)\n", qerr);
        } else if (qerr <= 30.0) {
          std::printf("Color quantization applied. Error ratio: %.2f (average)\n", qerr);
        } else if (qerr < 75.0) {
          std::printf("Color quantization applied. Error ratio: %.2f (bad!)\n", qerr);
        } else {
          std::printf("Color quantization applied. Error ratio: %.2f (awful!!!)\n", qerr);
        }
      }
    }
    reorderColors(palette, 256, FMT_ABGR, FMT_ARGB);

    return size;
  }
  return 0;
}


uint32_t Colors::padBlock(uint8_t *src, uint8_t *dst, unsigned width, unsigned height,
                          unsigned newWidth, unsigned newHeight) noexcept
{
  if (src != nullptr && dst != nullptr && width > 0 && height > 0 && newWidth >= width &&
      newHeight >= height && (newWidth & 3) == 0 && (newHeight & 3) == 0) {

    for (unsigned y = 0; y < height; y++) {
      for (unsigned x = 0; x < width; x++, src += 4, dst += 4) {
        dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
      }
      // padding horizontally with previously used values
      for (unsigned x = width; x < newWidth; x++, dst += 4) {
        dst[0] = dst[-4]; dst[1] = dst[-3]; dst[2] = dst[-2]; dst[3] = dst[-1];
      }
    }

    // padding vertically with previously used values
    int shift = -(newWidth * 4);  // offset into previous line
    for (unsigned y = height; y < newHeight; y++) {
      for (unsigned x = 0; x < newWidth; x++, dst += 4) {
        dst[0] = dst[shift+0]; dst[1] = dst[shift+1]; dst[2] = dst[shift+2]; dst[3] = dst[shift+3];
      }
    }

    return newWidth*newHeight;
  }
  return 0;
}


uint32_t Colors::unpadBlock(uint8_t *src, uint8_t *dst, unsigned width, unsigned height,
                            unsigned newWidth, unsigned newHeight) noexcept
{
  if (src != nullptr && dst != nullptr && width > 0 && height > 0 &&
      newWidth > 0 && newHeight > 0 &&
      newWidth <= width && newHeight <= height &&
      (width & 3) == 0 && (height & 3) == 0) {

    for (unsigned y = 0; y < newHeight; y++, src += (width-newWidth)*4) {
      for (unsigned x = 0; x < newWidth; x++, src += 4, dst += 4) {
        dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
      }
    }
    return newWidth*newHeight;
  }
  return 0;
}


uint32_t Colors::reorderColors(uint8_t *buffer, uint32_t size,
                               ColorFormat from, ColorFormat to) noexcept
{
  if (buffer != nullptr && size > 0) {
    int c0 = 0, c1 = 0, c2 = 0, c3 = 0;   // defines how much to shift each component
    switch (from) {
      case FMT_ARGB:
        switch (to) {
          case FMT_ABGR: c0 = 16; c2 = -16; break;
          case FMT_BGRA: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          case FMT_RGBA: c0 = 8; c1 = 8; c2 = 8; c3 = -24; break;
          default: break;
        }
        break;
      case FMT_ABGR:
        switch (to) {
          case FMT_ARGB: c0 = 16; c2 = -16; break;
          case FMT_BGRA: c0 = 8; c1 = 8; c2 = 8; c3 = -24; break;
          case FMT_RGBA: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          default: break;
        }
        break;
      case FMT_BGRA:
        switch (to) {
          case FMT_ARGB: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          case FMT_ABGR: c0 = 24; c1 = -8; c2 = -8; c3 = -8; break;
          case FMT_RGBA: c1 = 16; c3 = -16; break;
          default: break;
        }
        break;
      case FMT_RGBA:
        switch (to) {
          case FMT_ARGB: c0 = 24; c1 = -8; c2 = -8; c3 = -8; break;
          case FMT_ABGR: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          case FMT_BGRA: c1 = 16; c3 = -16; break;
          default: break;
        }
        break;
    }

    uint32_t* src = (uint32_t*)buffer;
    for (uint32_t i = 0; i < size; i++, src++) {
      uint32_t srcPixel = get32u(src);
      uint32_t dstPixel = 0;
      dstPixel |= (srcPixel & 0x000000ff) << c0;
      dstPixel |= (c1 < 0) ? (srcPixel & 0x0000ff00) >> c1 : (srcPixel & 0x0000ff00) << c1;
      dstPixel |= (c2 < 0) ? (srcPixel & 0x00ff0000) >> c2 : (srcPixel & 0x00ff0000) << c2;
      dstPixel |= (srcPixel & 0xff000000) >> c3;
      *src = get32u(&dstPixel);
    }
    return size;
  }
  return 0;
}

