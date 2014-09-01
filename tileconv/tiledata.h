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
#ifndef _TILEDATA_H_
#define _TILEDATA_H_
#include <cstdint>
#include <string>
#include <functional>
#include "types.h"

/** Data structure needed to process individual tiles independently */
struct TileData {
  TileData(bool encode, int idx, BytePtr indexed, BytePtr palette, BytePtr deflated,
           int width, int height, unsigned type, unsigned deflatedSize) noexcept;

  bool isEncoding;        // indicates whether this data is used for encoding (true) or decoding (false)
  BytePtr ptrIndexed;     // storage for indexed tile (encoding: in, decoding out)
  BytePtr ptrPalette;     // storage for palette (encoding: in, decoding: out)
  BytePtr ptrDeflated;    // storage for compressed tile (encoding: out, decoding: in)
  int index;              // the tile index/serial number (starting at 0)
  int tileWidth;          // width of the tile (encoding: in, decoding: out)
  int tileHeight;         // height of the tile (encoding: in, decoding: out)
  uint32_t size;          // data size (encoding: deflated size, decoding input: deflated size, decoding output: size of palette+indexed tile, error: 0)
  uint32_t encodingType;  // encoding type (needed for decoding)
  bool error;             // true if an error occurred
  std::string errorMsg;   // Contains a descriptive message if an error occurred
};

/** Shared pointer type for TileData. */
typedef std::shared_ptr<TileData> TileDataPtr;


/** Function object needed for priority queue. */
namespace std {
  template<> struct greater<TileDataPtr> {
    bool operator()(const TileDataPtr &lhs, const TileDataPtr &rhs) const noexcept
    {
      if (lhs != nullptr && rhs != nullptr) {
        return lhs->index > rhs->index;
      } else {
        return false;
      }
    }
  };
}


#endif		// _TILEDATA_H_
