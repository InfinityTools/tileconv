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
#include <cstring>
#include "funcs.h"
#include "dxtbase.h"


DxtBase::DxtBase() noexcept
: m_colorFormat(ColorFormat::ARGB)
, m_flags()
{
}


DxtBase::DxtBase(ColorFormat format) noexcept
: m_colorFormat(format)
, m_flags()
{
}


DxtBase::DxtBase(ColorFormat format, int flags) noexcept
: m_colorFormat(format)
, m_flags(flags)
{
}


DxtBase::~DxtBase() noexcept
{
}


bool DxtBase::decompressBlock(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    if (isDxt1()) {
      if (!DecodeBlockDxt1(src, dst)) return false;
    } else if (isDxt3()) {
      if (!DecodeBlockDxt3(src, dst)) return false;
    } else if (isDxt5()) {
      if (!DecodeBlockDxt5(src, dst)) return false;
    } else {
      return false;
    }
    DxtBase::ReorderColors(dst, 16, ColorFormat::ARGB, getColorFormat());
    return true;
  }
  return false;
}


bool DxtBase::compressImage(uint8_t *src, uint8_t *dst, unsigned width, unsigned height) noexcept
{
  if (src != nullptr && dst != nullptr && width > 0 && height > 0) {
    uint8_t  block[64];
    uint8_t  paddedBlock[64];
    unsigned dstBlockSize = getRequiredSpace(4, 4);
    unsigned stride = width << 2;
    unsigned srcOfs = 0;
    unsigned dstOfs = 0;

    for (unsigned y = 0; y < height; y += 4) {
      unsigned bh = std::min(4u, height-y);
      for (unsigned x = 0; x < width; x += 4) {
        unsigned bw = std::min(4u, width-x);
        for (unsigned by = 0, bofs = 0, sofs = srcOfs; by < bh; by++, bofs += bw << 2, sofs += stride) {
          std::memcpy(&block[bofs], src+sofs, bw << 2);
        }
        padBlock(block, paddedBlock, bw, bh, 4, 4, false);
        if (!compressBlock(paddedBlock, dst+dstOfs)) return false;

        srcOfs += bw << 2;
        dstOfs += dstBlockSize;
      }
      srcOfs += 3*stride;
    }

    return true;
  }
  return false;
}


bool DxtBase::decompressImage(uint8_t *src, uint8_t *dst, unsigned width, unsigned height) noexcept
{
  if (src != nullptr && dst != nullptr && width > 0 && height > 0) {
    uint8_t block[64];
    uint8_t paddedBlock[64];
    unsigned srcBlockSize = getRequiredSpace(4, 4);
    unsigned stride = width << 2;
    unsigned srcOfs = 0;
    unsigned dstOfs = 0;

    for (unsigned y = 0; y < height; y += 4) {
      unsigned bh = std::min(4u, height-y);
      for (unsigned x = 0; x < width; x += 4) {
        unsigned bw = std::min(4u, width-x);
        if (!decompressBlock(src+srcOfs, paddedBlock)) return false;
        unpadBlock(paddedBlock, block, 4, 4, bw, bh);
        for (unsigned by = 0, bofs = 0, dofs = dstOfs; by < bh; by++, bofs += bw << 2, dofs += stride) {
          std::memcpy(dst+dofs, &block[bofs], bw << 2);
        }

        srcOfs += srcBlockSize;
        dstOfs += bw << 2;
      }
      dstOfs += 3*stride;
    }
    return true;
  }
  return false;
}


unsigned DxtBase::padBlock(const uint8_t *src, uint8_t *dst, unsigned srcWidth, unsigned srcHeight,
                           unsigned dstWidth, unsigned dstHeight, bool useCopy) noexcept
{
  if (src != nullptr && dst != nullptr && srcWidth > 0 && srcHeight > 0 &&
      dstWidth >= srcWidth && dstHeight >= srcHeight) {

    for (unsigned y = 0; y < srcHeight; y++) {
      for (unsigned x = 0; x < srcWidth; x++, src += 4, dst += 4) {
        dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
      }
      // padding horizontally with previously used values
      for (unsigned x = srcWidth; x < dstWidth; x++, dst += 4) {
        if (useCopy) {
          dst[0] = dst[-4]; dst[1] = dst[-3]; dst[2] = dst[-2]; dst[3] = dst[-1];
        } else {
          dst[0] = dst[1] = dst[2] = dst[3] = 0;
        }
      }
    }

    // padding vertically with previously used values
    int shift = -(dstWidth * 4);  // offset into previous line
    for (unsigned y = srcHeight; y < dstHeight; y++) {
      for (unsigned x = 0; x < dstWidth; x++, dst += 4) {
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


unsigned DxtBase::unpadBlock(const uint8_t *src, uint8_t *dst, unsigned srcWidth, unsigned srcHeight,
                             unsigned dstWidth, unsigned dstHeight) noexcept
{
  if (src != nullptr && dst != nullptr && dstWidth > 0 && dstHeight > 0 &&
      dstWidth <= srcWidth && dstHeight <= srcHeight) {

    unsigned srcStep = (srcWidth-dstWidth)*4;
    for (unsigned y = 0; y < dstHeight; y++, src += srcStep) {
      for (unsigned x = 0; x < dstWidth; x++, src += 4, dst += 4) {
        dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
      }
    }

    return dstWidth*dstHeight;
  }
  return 0;
}


int DxtBase::getRequiredSpace(int width, int height) const noexcept
{
  int blockNum = ((width+3) & ~3)*((height+3) & ~3) / 16;
  if (isDxt1()) {
    return 8*blockNum;
  } else if (isDxt3() || isDxt5()) {
    return 16*blockNum;
  } else {
    return 0;
  }
}


// ------------------------------ Static methods ------------------------------

bool DxtBase::DecodeBlockDxt1(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    uint8_t block[8];
    uint16_t c0 = get16u((uint16_t*)(src));
    uint16_t c1 = get16u((uint16_t*)(&src[2]));
    if (!UnpackColors565(src, block)) return false;
    int code = get32u((uint32_t*)(&src[4]));
    for (unsigned idx = 0; idx < 16; idx++, code >>= 2, dst += 4) {
      switch (code & 3) {
        case 0:
          // 100% c0, 0% c1
          dst[0] = block[0]; dst[1] = block[1]; dst[2] = block[2]; dst[3] = 255;
          break;
        case 1:
          // 0% c0, 100% c1
          dst[0] = block[4]; dst[1] = block[5]; dst[2] = block[6]; dst[3] = 255;
          break;
        case 2:
          if (c0 > c1) {
            // 66% c0, 33% c1
            dst[0] = ((block[0] << 1) + block[4]) / 3;
            dst[1] = ((block[1] << 1) + block[5]) / 3;
            dst[2] = ((block[2] << 1) + block[6]) / 3;
            dst[3] = 255;
          } else {
            // 50% c0, 50% c1
            dst[0] = (block[0] + block[4]) >> 1;
            dst[1] = (block[1] + block[5]) >> 1;
            dst[2] = (block[2] + block[6]) >> 1;
            dst[3] = 255;
          }
          break;
        case 3:
          if (c0 > c1) {
            // 33% c0, 66% c1
            dst[0] = (block[0] + (block[4] << 1)) / 3;
            dst[1] = (block[1] + (block[5] << 1)) / 3;
            dst[2] = (block[2] + (block[6] << 1)) / 3;
            dst[3] = 255;
          } else {
            // transparent
            dst[0] = dst[1] = dst[2] = dst[3] = 0;
          }
          break;
        default:
          break;
      }
    }

    return true;
  }
  return false;
}


bool DxtBase::DecodeBlockDxt3(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    uint8_t block[8];
    uint64_t alpha = get64u((uint64_t*)src);
    if (!UnpackColors565(&src[8], block)) return false;
    int code = get32u((uint32_t*)(&src[12]));
    for (unsigned idx = 0; idx < 16; idx++, code >>= 2, alpha >>= 4, dst += 4) {
      // pre-setting alpha
      dst[3] = (uint8_t)((alpha & 0x0f) | (alpha & 0x0f) << 4);
      switch (code & 3) {
        case 0:
          // 100% c0, 0% c1
          dst[0] = block[0]; dst[1] = block[1]; dst[2] = block[2];
          break;
        case 1:
          // 0% c0, 100% c1
          dst[0] = block[4]; dst[1] = block[5]; dst[2] = block[6];
          break;
        case 2:
          // 66% c0, 33% c1
          dst[0] = ((block[0] << 1) + block[4]) / 3;
          dst[1] = ((block[1] << 1) + block[5]) / 3;
          dst[2] = ((block[2] << 1) + block[6]) / 3;
          break;
        case 3:
          // 33% c0, 66% c1
          dst[0] = (block[0] + (block[4] << 1)) / 3;
          dst[1] = (block[1] + (block[5] << 1)) / 3;
          dst[2] = (block[2] + (block[6] << 1)) / 3;
          break;
        default:
          break;
      }
    }

    return true;
  }
  return false;
}


bool DxtBase::DecodeBlockDxt5(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    uint8_t block[8];
    // generating alpha table
    uint64_t ctrl = get64u((uint64_t*)&src[2]);
    uint8_t alpha[8];
    alpha[0] = (uint8_t)(ctrl);
    ctrl >>= 8;
    alpha[1] = (uint8_t)(ctrl);
    ctrl >>= 8;
    if (alpha[0] > alpha[1]) {
      alpha[2] = (6*(uint32_t)alpha[0] +   (uint32_t)alpha[1]) / 7;
      alpha[3] = (5*(uint32_t)alpha[0] + 2*(uint32_t)alpha[1]) / 7;
      alpha[4] = (4*(uint32_t)alpha[0] + 3*(uint32_t)alpha[1]) / 7;
      alpha[5] = (3*(uint32_t)alpha[0] + 4*(uint32_t)alpha[1]) / 7;
      alpha[6] = (2*(uint32_t)alpha[0] + 5*(uint32_t)alpha[1]) / 7;
      alpha[7] = (  (uint32_t)alpha[0] + 6*(uint32_t)alpha[1]) / 7;
    } else {
      alpha[2] = (4*(uint32_t)alpha[0] +   (uint32_t)alpha[1]) / 5;
      alpha[3] = (3*(uint32_t)alpha[0] + 2*(uint32_t)alpha[1]) / 5;
      alpha[4] = (2*(uint32_t)alpha[0] + 3*(uint32_t)alpha[1]) / 5;
      alpha[5] = (  (uint32_t)alpha[0] + 4*(uint32_t)alpha[1]) / 5;
      alpha[6] = 0;
      alpha[7] = 255;
    }

    if (!UnpackColors565(&src[8], block)) return false;
    int code = get32u((uint32_t*)(&src[12]));
    for (unsigned idx = 0; idx < 16; idx++, code >>= 2, ctrl >>= 3, dst += 4) {
      // pre-setting alpha
      dst[3] = alpha[(size_t)(ctrl & 7UL)];
      switch (code & 3) {
        case 0:
          // 100% c0, 0% c1
          dst[0] = block[0]; dst[1] = block[1]; dst[2] = block[2];
          break;
        case 1:
          // 0% c0, 100% c1
          dst[0] = block[4]; dst[1] = block[5]; dst[2] = block[6];
          break;
        case 2:
          // 66% c0, 33% c1
          dst[0] = ((block[0] << 1) + block[4]) / 3;
          dst[1] = ((block[1] << 1) + block[5]) / 3;
          dst[2] = ((block[2] << 1) + block[6]) / 3;
          break;
        case 3:
          // 33% c0, 66% c1
          dst[0] = (block[0] + (block[4] << 1)) / 3;
          dst[1] = (block[1] + (block[5] << 1)) / 3;
          dst[2] = (block[2] + (block[6] << 1)) / 3;
          break;
        default:
          break;
      }
    }

    return true;
  }
  return false;
}


bool DxtBase::UnpackColors565(const uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    uint16_t c1 = get16u((uint16_t*)(&src[0]));
    uint16_t c2 = get16u((uint16_t*)(&src[2]));

    dst[0] = ((c1 << 3) & 0xf8) | ((c1 >> 2) & 0x07);
    dst[1] = ((c1 >> 3) & 0xfc) | ((c1 >> 9) & 0x03);
    dst[2] = ((c1 >> 8) & 0xf8) | ((c1 >> 13) & 0x07);
    dst[4] = ((c2 << 3) & 0xf8) | ((c2 >> 2) & 0x07);
    dst[5] = ((c2 >> 3) & 0xfc) | ((c2 >> 9) & 0x03);
    dst[6] = ((c2 >> 8) & 0xf8) | ((c2 >> 13) & 0x07);
    dst[3] = dst[7] = 255;

    return true;
  }
  return false;
}


bool DxtBase::ReorderColors(uint8_t *buffer, unsigned size, ColorFormat from, ColorFormat to) noexcept
{
  if (buffer != nullptr && size >= 0) {
    int c0 = 0, c1 = 0, c2 = 0, c3 = 0;   // defines how much to shift each component
    switch (from) {
      case ColorFormat::ARGB:
        switch (to) {
          case ColorFormat::ABGR: c0 = 16; c2 = -16; break;
          case ColorFormat::BGRA: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          case ColorFormat::RGBA: c0 = 8; c1 = 8; c2 = 8; c3 = -24; break;
          default: break;
        }
        break;
      case ColorFormat::ABGR:
        switch (to) {
          case ColorFormat::ARGB: c0 = 16; c2 = -16; break;
          case ColorFormat::BGRA: c0 = 8; c1 = 8; c2 = 8; c3 = -24; break;
          case ColorFormat::RGBA: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          default: break;
        }
        break;
      case ColorFormat::BGRA:
        switch (to) {
          case ColorFormat::ARGB: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          case ColorFormat::ABGR: c0 = 24; c1 = -8; c2 = -8; c3 = -8; break;
          case ColorFormat::RGBA: c1 = 16; c3 = -16; break;
          default: break;
        }
        break;
      case ColorFormat::RGBA:
        switch (to) {
          case ColorFormat::ARGB: c0 = 24; c1 = -8; c2 = -8; c3 = -8; break;
          case ColorFormat::ABGR: c0 = 24; c1 = 8; c2 = -8; c3 = -24; break;
          case ColorFormat::BGRA: c1 = 16; c3 = -16; break;
          default: break;
        }
        break;
    }

    uint32_t *src = (uint32_t*)buffer;
    for (unsigned i = 0; i < size; i++, src++) {
      uint32_t srcPixel = get32u(src);
      uint32_t dstPixel = 0;
      dstPixel |= (srcPixel & 0x000000ff) << c0;
      dstPixel |= (c1 < 0) ? (srcPixel & 0x0000ff00) >> c1 : (srcPixel & 0x0000ff00) << c1;
      dstPixel |= (c2 < 0) ? (srcPixel & 0x00ff0000) >> c2 : (srcPixel & 0x00ff0000) << c2;
      dstPixel |= (srcPixel & 0xff000000) >> c3;
      *src = get32u(&dstPixel);
    }

    return true;
  }
  return false;
}
