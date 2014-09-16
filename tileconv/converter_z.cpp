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
#include "colors.h"
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
  if (palette != nullptr && indexed != nullptr && encoded != nullptr) {
    setWidth(64); setHeight(64);    // only used in TIZ
    int size = get16u_be((uint16_t*)encoded); encoded += 2;
    if (size > 0) {
      Compression compression;
      int inflatedSize = 5120*2;
      BytePtr ptrInflated(new uint8_t[inflatedSize], std::default_delete<uint8_t[]>());
      inflatedSize = compression.inflate(encoded, size, ptrInflated.get(), inflatedSize);
      if (inflatedSize >= 5120) {
        std::memcpy(palette, ptrInflated.get(), 1024);
        std::memcpy(indexed, ptrInflated.get()+1024, 4096);
        return 5120;    // fixed tile size
      }
    }
  }
  return 0;
}


int ConverterZ::decodeTile1(uint8_t *palette, uint8_t *indexed, uint8_t *encoded) noexcept
{
  if (palette != nullptr && indexed != nullptr && encoded != nullptr) {
    int tileSize = get16u_be((uint16_t*)encoded); encoded += 2;
    int maskSize = get16u_be((uint16_t*)encoded); encoded += 2;
    int dataSize = tileSize - maskSize - 2;
    uint8_t *mask = encoded;
    uint8_t *data = encoded + maskSize;
    if (dataSize > 0 && maskSize > 0) {
      Compression compression;
      // storage for RGB palette and 512 byte alpha bitmask
      BytePtr ptrInflated(new uint8_t[1280], std::default_delete<uint8_t[]>());
      maskSize = compression.inflate(mask, maskSize, ptrInflated.get(), 1280);
      mask = ptrInflated.get();
      if (maskSize >= 1280) {
        // skipping stored palette
        mask += 768;    // now pointing to 512 bytes alpha bitmask

        // decompressing JPEG data
        Jpeg jpeg;
        if (!jpeg.updateInformation(data, dataSize)) return 0;
        if (jpeg.getWidth() > 0 && jpeg.getHeight() > 0) {
          setWidth(jpeg.getWidth()); setHeight(jpeg.getHeight());
          BytePtr ptrARGB(new uint8_t[getWidth()*getHeight()*4], std::default_delete<uint8_t[]>());
          if (jpeg.decompress(data, dataSize, ptrARGB.get(), Jpeg::PF_RGBA, 0)) {
            // applying alpha mask to ARGB data
            uint8_t *argb = ptrARGB.get();
            for (int i = 0; i < getWidth()*getHeight(); i++, argb += 4) {
              int mofs = i >> 3;          // mask byte offset
              int mbit = 7 - (i & 7);    // counting from MSB
              if (((mask[mofs] >> mbit) & 1) == 0) {
                // transparent pixel found
                argb[0] = argb[1] = argb[2] = argb[3] = 0;
              }
            }

            // reducing colors
            Colors colors(getOptions());
            if (colors.ARGBToPal(ptrARGB.get(), indexed, palette,
                                 getWidth(), getHeight()) == getWidth()*getHeight()) {
              ReorderColors(palette, 256, ColorFormat::ARGB, getColorFormat());
              return 1024 + getWidth()*getHeight();
            }
          }
        }
      }
    }
  }
  return 0;
}


int ConverterZ::decodeTile2(uint8_t *palette, uint8_t *indexed, uint8_t *encoded) noexcept
{
  if (palette != nullptr && indexed != nullptr && encoded != nullptr) {
    int dataSize = get16u_be((uint16_t*)encoded); encoded += 2;
    if (dataSize > 0) {
      Jpeg jpeg;
      if (!jpeg.updateInformation(encoded, dataSize)) return 0;
      if (jpeg.getWidth() > 0 && jpeg.getHeight() > 0) {
        setWidth(jpeg.getWidth()); setHeight(jpeg.getHeight());
        BytePtr ptrARGB(new uint8_t[getWidth()*getHeight()*4], std::default_delete<uint8_t[]>());
        if (jpeg.decompress(encoded, dataSize, ptrARGB.get(), Jpeg::PF_RGBA, 0)) {
          // reducing colors
          Colors colors(getOptions());
          if (colors.ARGBToPal(ptrARGB.get(), indexed, palette,
                               getWidth(), getHeight()) == getWidth()*getHeight()) {
            ReorderColors(palette, 256, ColorFormat::ARGB, getColorFormat());
            return 1024 + getWidth()*getHeight();
          }
        }
      }
    }
  }
  return 0;
}


bool ConverterZ::isTypeValid() const noexcept
{
  return (Options::GetEncodingType(getType()) == Encoding::Z);
}

}   // namespace tc
