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
#ifndef _CONVERTER_DXT_H_
#define _CONVERTER_DXT_H_
#include "converter.h"

namespace tc {

/** Implements a DXTn encoder and decoder. */
class ConverterDxt : public Converter
{
public:
  ConverterDxt(const Options& options, unsigned type) noexcept;
  ~ConverterDxt() noexcept;

  bool canEncode() const noexcept { return true; }
  bool canDecode() const noexcept { return true; }
  bool deflateAllowed() const noexcept { return true; }

  /** See Converter::getRequiredSpace() */
  int getRequiredSpace(int width, int height) const noexcept;

  /** Returns the value expanded to the nearest multiple of 4. */
  int getPaddedValue(int v) const noexcept;

  /** See Converter::convert() */
  int convert(uint8_t *palette, uint8_t *indexed, uint8_t *encoded, int width, int height) noexcept;

protected:
  // See Converter::isTypeValid()
  bool isTypeValid() const noexcept;

private:

  // Performs pixel encoding/decoding on the given graphics data
  int encodeTile(uint8_t *src, uint8_t *dst, int width, int height) noexcept;
  int decodeTile(uint8_t *src, uint8_t *dst, int width, int height) noexcept;

  // Returns squish flags based on type and quality.
  int getFlags() const noexcept;

  // Encode a single DXTn block
  bool compressBlock(uint8_t *src, uint8_t *dst) noexcept;
  // Decode a single DXTn block
  bool decompressBlock(uint8_t *src, uint8_t *dst) noexcept;

  // tileconv's own decoding routines as fallback solution. Decodes into ColorFormat::ARGB.
  bool decodeBlockDxt1(uint8_t *src, uint8_t *dst) noexcept;
  bool decodeBlockDxt3(uint8_t *src, uint8_t *dst) noexcept;
  bool decodeBlockDxt5(uint8_t *src, uint8_t *dst) noexcept;

  // Unpacks two consecutive 16-bit RGB565 colors into 32-bit ColorFormat::ARGB.
  bool unpackColors565(const uint8_t *src, uint8_t *dst) noexcept;
};

}   // namespace tc

#endif		// _CONVERTER_DXT_H_
