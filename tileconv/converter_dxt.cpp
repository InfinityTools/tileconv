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
#include <squish.h>
#include "funcs.h"
#include "colors.h"
#include "converter_dxt.h"

namespace tc {

ConverterDxt::ConverterDxt(const Options& options, unsigned type) noexcept
: Converter(options, type)
{
}


ConverterDxt::~ConverterDxt() noexcept
{
}


int ConverterDxt::getRequiredSpace(int width, int height) const noexcept
{
  if (width > 0 && height > 0) {
    int blockNum = (getPaddedValue(width)*getPaddedValue(height)) >> 4;
    switch (getType()) {
      case 1:
        return blockNum << 3;
      case 2:
      case 3:
        return blockNum << 4;
      default:
        break;
    }
  }
  return 0;
}


int ConverterDxt::getPaddedValue(int v) const noexcept
{
  return (v + 3) & ~3;
}


int ConverterDxt::convert(uint8_t *palette, uint8_t *indexed, uint8_t *encoded, int width, int height) noexcept
{
  if (palette != nullptr && indexed != nullptr && encoded != nullptr) {
    Colors colors(getOptions());

    if (isEncoding() && width > 0 && height > 0) {
      // Paletted -> Encoded
      setWidth(width); setHeight(height);
      BytePtr ptrARGB(new uint8_t[getWidth()*getHeight()*4], std::default_delete<uint8_t[]>());
      ReorderColors(palette, 256, getColorFormat(), ColorFormat::ARGB);
      colors.palToARGB(indexed, palette, ptrARGB.get(), getWidth()*getHeight());
      ReorderColors(ptrARGB.get(), getWidth()*getHeight(), ColorFormat::ARGB, getColorFormat());
      return encodeTile(ptrARGB.get(), encoded, getWidth(), getHeight());
    } else if (!isEncoding()) {
      // Encoded -> Paletted
      setWidth(get16u_le((uint16_t*)encoded)); encoded += 2;
      setHeight(get16u_le((uint16_t*)encoded)); encoded += 2;
      BytePtr ptrARGB(new uint8_t[getWidth()*getHeight()*4], std::default_delete<uint8_t[]>());
      if (decodeTile(encoded, ptrARGB.get(), getWidth(), getHeight()) > 0) {
        ReorderColors(ptrARGB.get(), getWidth()*getHeight(), getColorFormat(), ColorFormat::ARGB);
        if (colors.ARGBToPal(ptrARGB.get(), indexed, palette, getWidth(), getHeight()) == getWidth()*getHeight()) {
          return 1024 + getWidth()*getHeight();
        }
      }
    }
  }
  return 0;
}


bool ConverterDxt::isTypeValid() const noexcept
{
  switch (Options::GetEncodingType(getType())) {
    case Encoding::BC1:
    case Encoding::BC2:
    case Encoding::BC3:
      return true;
    default:
      break;
  }
  return false;
}


int ConverterDxt::encodeTile(uint8_t *src, uint8_t *dst, int width, int height) noexcept
{
  if (isEncoding() && src != nullptr && dst != nullptr && width > 0 && height > 0) {
    uint8_t  block[64];
    uint8_t  paddedBlock[64];
    int blockSize = getRequiredSpace(4, 4);
    int stride;
    int srcOfs = 0;
    int dstOfs = 0;
    uint16_t v16;

    // initializing pixel encoding header
    v16 = (uint16_t)width; *((uint16_t*)dst) = get16u_le(&v16); dst += 2;
    v16 = (uint16_t)height; *((uint16_t*)dst) = get16u_le(&v16); dst += 2;

    // encoding graphics
    stride = getWidth() << 2;
    for (int y = 0; y < getHeight(); y += 4) {
      int bh = std::min(4, getHeight()-y);
      for (int x = 0; x < getWidth(); x += 4) {
        int bw = std::min(4, getWidth()-x);
        for (int by = 0, bofs = 0, sofs = srcOfs; by < bh; by++, bofs += bw << 2, sofs += stride) {
          std::memcpy(&block[bofs], src+sofs, bw << 2);
        }
        PadBlock(block, paddedBlock, bw, bh, 4, 4, false);
        if (!compressBlock(paddedBlock, dst+dstOfs)) return false;

        srcOfs += bw << 2;
        dstOfs += blockSize;
      }
      srcOfs += 3*stride;
    }
    return getRequiredSpace(getWidth(), getHeight()) + HEADER_TILE_ENCODED_SIZE;
  }
  return 0;
}


int ConverterDxt::decodeTile(uint8_t *src, uint8_t *dst, int width, int height) noexcept
{
  if (!isEncoding() && src != nullptr && dst != nullptr && width > 0 && height > 0) {
    uint8_t  block[64];
    uint8_t  paddedBlock[64];
    int blockSize = getRequiredSpace(4, 4);
    int stride = width << 2;
    int srcOfs = 0;
    int dstOfs = 0;

    // decoding blocks
    for (int y = 0; y < height; y += 4) {
      int bh = std::min(4, height-y);
      for (int x = 0; x < width; x += 4) {
        int bw = std::min(4, width-x);
        if (!decompressBlock(src+srcOfs, paddedBlock)) return false;
        UnpadBlock(paddedBlock, block, 4, 4, bw, bh);
        for (int by = 0, bofs = 0, dofs = dstOfs; by < bh; by++, bofs += bw << 2, dofs += stride) {
          std::memcpy(dst+dofs, &block[bofs], bw << 2);
        }

        srcOfs += blockSize;
        dstOfs += bw << 2;
      }
      dstOfs += 3*stride;
    }
    return width*height*4;
  }
  return 0;
}


int ConverterDxt::getFlags() const noexcept
{
  int retVal = 0;

  // setting encoding type
  switch (getType()) {
    case 1: retVal = squish::kDxt1; break;
    case 2: retVal = squish::kDxt3; break;
    case 3: retVal = squish::kDxt5; break;
    default: return retVal;
  }

  if (isEncoding()) {
    // setting encoding quality
    switch (getOptions().getEncodingQuality()) {
      case 0: case 1: case 2:
        retVal |= squish::kColourRangeFit;
        break;
      case 3: case 4:
        retVal |= squish::kColourClusterFit;
        break;
      case 5: case 6:
        retVal |= squish::kColourClusterFit | squish::kWeightColourByAlpha;
        break;
      case 7: case 8: case 9:
        retVal |= squish::kColourIterativeClusterFit | squish::kWeightColourByAlpha;
        break;
      default:
        break;
    }
  }

  return retVal;
}


bool ConverterDxt::compressBlock(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    uint8_t buffer[64];
    std::memcpy(buffer, src, 64);
    ReorderColors(buffer, 16, getColorFormat(), ColorFormat::ABGR);
    squish::Compress(buffer, dst, getFlags());
    return true;
  }
  return false;
}


bool ConverterDxt::decompressBlock(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    switch (getType()) {
      case 1:
        if (!decodeBlockDxt1(src, dst)) return false;
        break;
      case 2:
        if (!decodeBlockDxt3(src, dst)) return false;
        break;
      case 3:
        if (!decodeBlockDxt5(src, dst)) return false;
        break;
      default:
        return false;
    }
    ReorderColors(dst, 16, ColorFormat::ARGB, getColorFormat());
    return true;
  }
  return false;
}


bool ConverterDxt::decodeBlockDxt1(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    uint8_t block[8];
    uint16_t c0 = get16u_le((uint16_t*)(src));
    uint16_t c1 = get16u_le((uint16_t*)(&src[2]));
    if (!unpackColors565(src, block)) return false;
    int code = get32u_le((uint32_t*)(&src[4]));
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


bool ConverterDxt::decodeBlockDxt3(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    uint8_t block[8];
    uint64_t alpha = get64u_le((uint64_t*)src);
    if (!unpackColors565(&src[8], block)) return false;
    int code = get32u_le((uint32_t*)(&src[12]));
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


bool ConverterDxt::decodeBlockDxt5(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    uint8_t block[8];
    // generating alpha table
    uint64_t ctrl = get64u_le((uint64_t*)&src[2]);
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

    if (!unpackColors565(&src[8], block)) return false;
    int code = get32u_le((uint32_t*)(&src[12]));
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


bool ConverterDxt::unpackColors565(const uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    uint16_t c1 = get16u_le((uint16_t*)(&src[0]));
    uint16_t c2 = get16u_le((uint16_t*)(&src[2]));

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


}   // namespace tc
