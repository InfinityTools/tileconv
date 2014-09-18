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
#ifndef _CONVERTER_Z_H_
#define _CONVERTER_Z_H_
#include "converter.h"

namespace tc {

/** Implements a TIZ/MOZ decoder. */
class ConverterZ: public Converter
{
public:
  ConverterZ(const Options& options, unsigned type) noexcept;
  ~ConverterZ() noexcept;

  bool canEncode() const noexcept { return false; }
  bool canDecode() const noexcept { return true; }
  bool deflateAllowed() const noexcept { return false; }

  /** Returns the max. possible space required to hold encoded tile data. */
  int getRequiredSpace(int width, int height) const noexcept;

  /** See Converter::convert() */
  int convert(uint8_t *palette, uint8_t *indexed, uint8_t *encoded, int width, int height) noexcept;

protected:
  // Decoding methods for each tile type
  int decodeTile0(uint8_t *palette, uint8_t *indexed, uint8_t *encoded) noexcept;
  int decodeTile1(uint8_t *palette, uint8_t *indexed, uint8_t *encoded) noexcept;
  int decodeTile2(uint8_t *palette, uint8_t *indexed, uint8_t *encoded) noexcept;

  // Apply Tile1 alpha bitmask to indexed pixels
  void applyAlpha(uint8_t *alpha, uint8_t *indexed, int size) noexcept;

  // See Converter::isTypeValid()
  bool isTypeValid() const noexcept;
};

}   // namespace tc

#endif		// _CONVERTER_Z_H_
