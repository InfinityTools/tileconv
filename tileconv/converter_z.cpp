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
#include "jpeg.h"
#include "funcs.h"
#include "compress.h"
#include "graphics.h"
#include "converter_z.h"

namespace tc {

ConverterZ::ConverterZ(const Options& options, unsigned type) noexcept
: Converter(options, type)
{
}


ConverterZ::~ConverterZ() noexcept
{
}


int ConverterZ::getRequiredSpace(int width, int height) const noexcept
{
  if (width > 0 && height > 0) {
    int sum = 0;
    for (int y = 0; y < height; y += 64) {
      int ty = std::max(1, std::min(64, height - y));
      for (int x = 0; x < width; x += 64) {
        int tx = std::max(1, std::min(64, width - x));
        int size = ty*tx;
        sum += 8 + 768 + ((size+7) >> 3) + size*3;  // using TIL1 structure as reference
      }
    }
    return sum;
  }
  return 0;
}


int ConverterZ::convert(uint8_t *palette, uint8_t *indexed, uint8_t *encoded, int width, int height) noexcept
{
  if (palette != nullptr && indexed != nullptr && encoded != nullptr) {
    if (!isEncoding()) {
      if (std::strncmp((char*)encoded, Graphics::HEADER_TIL0_SIGNATURE, 4) == 0) {
        return decodeTile0(palette, indexed, encoded+4);
      } else if (std::strncmp((char*)encoded, Graphics::HEADER_TIL1_SIGNATURE, 4) == 0) {
        return decodeTile1(palette, indexed, encoded+4);
      } else if (std::strncmp((char*)encoded, Graphics::HEADER_TIL2_SIGNATURE, 4) == 0) {
        return decodeTile2(palette, indexed, encoded+4);
      }
    }
  }
  return 0;
}


int ConverterZ::decodeTile0(uint8_t *palette, uint8_t *indexed, uint8_t *encoded) noexcept
{
  static const int TILE_SIZE = 5120;
  if (palette != nullptr && indexed != nullptr && encoded != nullptr) {
    int size = get16u_be((uint16_t*)encoded); encoded += 2;
    if (size > 0) {
      Compression compression;
      BytePtr ptrInflated(new uint8_t[TILE_SIZE*2], std::default_delete<uint8_t[]>());
      if (compression.inflate(encoded, size, ptrInflated.get(), TILE_SIZE*2) == TILE_SIZE) {
        setWidth(64); setHeight(64);    // only used in TIZ
        std::memcpy(palette, ptrInflated.get(), PALETTE_SIZE);
        std::memcpy(indexed, ptrInflated.get()+PALETTE_SIZE, 4096);
        return TILE_SIZE;
      }
    }
  }
  return 0;
}


int ConverterZ::decodeTile1(uint8_t *palette, uint8_t *indexed, uint8_t *encoded) noexcept
{
  if (palette != nullptr && indexed != nullptr && encoded != nullptr) {
    int tileSize = get16u_be((uint16_t*)encoded); encoded += 2;
    int dataSize = get16u_be((uint16_t*)encoded); encoded += 2;
    int imgSize = tileSize - dataSize - 2;
    uint8_t *data = encoded;
    uint8_t *img = encoded + dataSize;
    uint8_t *r, *g, *b, *alpha;
    if (dataSize > 0 && imgSize > 0) {
      Compression compression;
      // storage for RGB palette and 512 byte alpha bitmask
      BytePtr ptrInflated(new uint8_t[768+512], std::default_delete<uint8_t[]>());
      r = ptrInflated.get();
      g = ptrInflated.get() + 256;
      b = ptrInflated.get() + 512;
      alpha = ptrInflated.get() + 768;
      if (compression.inflate(data, dataSize, ptrInflated.get(), 1280) == 1280) {
        Jpeg jpeg(getOptions());
        uint8_t *pal[3];
        pal[0] = r;
        pal[1] = g;
        pal[2] = b;
        unsigned size = jpeg.decompress(img, imgSize, pal, palette, indexed);
        if (size > PALETTE_SIZE) {
          setWidth(jpeg.getWidth()); setHeight(jpeg.getHeight());
          applyAlpha(alpha, indexed, size - PALETTE_SIZE);
          return size;
        }
      }
    }
  }
  return 0;
}


int ConverterZ::decodeTile2(uint8_t *palette, uint8_t *indexed, uint8_t *encoded) noexcept
{
  if (palette != nullptr && indexed != nullptr && encoded != nullptr) {
    int imgSize = get16u_be((uint16_t*)encoded); encoded += 2;
    Jpeg jpeg(getOptions());
    unsigned size = jpeg.decompress(encoded, imgSize, nullptr, palette, indexed);
    if (size > PALETTE_SIZE) {
      setWidth(jpeg.getWidth()); setHeight(jpeg.getHeight());
      return size;
    }
  }
  return 0;
}


void ConverterZ::applyAlpha(uint8_t *alpha, uint8_t *indexed, int size) noexcept
{
  if (alpha != nullptr && indexed != nullptr && size > 0) {
    for (int i = 0; i < size; i++, indexed++) {
      int mofs = i >> 3;        // mask byte offset
      int mbit = 7 - (i & 7);   // counting from MSB
      if (((alpha[mofs] >> mbit) & 1) == 0) {
        // transparent pixel found
        *indexed = 0;
      }
    }
  }
}


bool ConverterZ::isTypeValid() const noexcept
{
  return (Options::GetEncodingType(getType()) == Encoding::Z);
}

}   // namespace tc
