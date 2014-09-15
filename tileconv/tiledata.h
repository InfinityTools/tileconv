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
#include <string>
#include "types.h"
#include "options.h"

namespace tc {

class TileData
{
public:
  explicit TileData(const Options &options) noexcept;
  ~TileData() noexcept;

  // Process tile data.
  TileData& operator()() noexcept;

  /** Read-only access to Options methods. */
  const Options& getOptions() const noexcept { return m_options; }

  /** Indicates whether to encode or decode data, based on the current encoding type. */
  void setEncoding(bool b) noexcept { m_encoding = b; }
  bool isEncoding() const noexcept { return m_encoding; }

  /** The index associated with this tile data (>= 0). */
  void setIndex(int index) noexcept;
  int getIndex() const noexcept { return m_index; }

  /** Encoding type. */
  void setType(unsigned type) noexcept;
  unsigned getType() const noexcept { return m_type; }

  /** Storage for palette (encoding: in, decoding: out). */
  void setPaletteData(BytePtr palette) noexcept { m_ptrPalette = palette; }
  BytePtr getPaletteData() const noexcept { return m_ptrPalette; }

  /** Storage for indexed tile data (encoding: in, decoding: out). */
  void setIndexedData(BytePtr indexed) noexcept { m_ptrIndexed = indexed; }
  BytePtr getIndexedData() const noexcept { return m_ptrIndexed; }

  /** Storage for compressed tile data (encoding: out, decoding: in). */
  void setDeflatedData(BytePtr deflated) noexcept { m_ptrDeflated = deflated; }
  BytePtr getDeflatedData() const noexcept { return m_ptrDeflated; }

  /** Tile width (encoding: in, decoding: out). */
  void setWidth(int width) noexcept;
  int getWidth() const noexcept { return m_width; }

  /** Tile height (encoding: in, decoding: out). */
  void setHeight(int height) noexcept;
  int getHeight() const noexcept { return m_height; }

  /** Data size (encoding: deflated size, decoding input: deflated size, decoding output: sizeof palette+indexed, error: 0). */
  void setSize(int size) noexcept;
  int getSize() const noexcept { return m_size; }

  /** Return information on error. */
  bool isError() const noexcept { return m_error; }
  const std::string& getErrorMsg() const noexcept { return m_errorMsg; }

private:
  // Check if data is valid for the encoding or decoding process
  bool isValid() const noexcept;

  // Set error state
  void setError(bool b) noexcept { m_error = b; }
  void setErrorMsg(std::string s) { m_errorMsg = s; }

  // Encode/decode current tile
  void encode() noexcept;
  void decode() noexcept;

private:
  static const unsigned PALETTE_SIZE;
  static const unsigned MAX_TILE_SIZE_8;
  static const unsigned MAX_TILE_SIZE_32;

  const Options& m_options;   // read-only reference to options instance
  bool        m_encoding;
  bool        m_error;        // indicates if an error occurred
  BytePtr     m_ptrPalette;   // storage for palette (encoding: in, decoding: out)
  BytePtr     m_ptrIndexed;   // storage for indexed tile (encoding: in, decoding out)
  BytePtr     m_ptrDeflated;  // storage for compressed tile (encoding: out, decoding: in)
  int         m_index;        // the tile index/serial number (starting at 0)
  int         m_width;        // width of the tile (encoding: in, decoding: out)
  int         m_height;       // height of the tile (encoding: in, decoding: out)
  int         m_type;         // encoding type (needed for decoding)
  int         m_size;         // data size (encoding: deflated size, decoding input: deflated size, decoding output: size of palette+indexed tile, error: 0)
  std::string m_errorMsg;     // contains a descriptive message if an error occurred
};

typedef std::shared_ptr<TileData> TileDataPtr;

}   // namespace tc


/** Function object needed for priority queue. */
namespace std {
  template<> struct greater<tc::TileDataPtr> {
    bool operator()(const tc::TileDataPtr &lhs, const tc::TileDataPtr &rhs) const noexcept
    {
      if (lhs != nullptr && rhs != nullptr) {
        return lhs->getIndex() > rhs->getIndex();
      } else {
        return false;
      }
    }
  };
}


#endif		// _TILEDATA_H_
