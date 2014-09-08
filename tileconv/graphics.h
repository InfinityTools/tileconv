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
#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <string>
#include "types.h"
#include "options.h"
#include "fileio.h"
#include "tiledata.h"
#include "dxtbase.h"


/** Provides functions for converting between TIS/MOS <-> TBC/MBC */
class Graphics
{
public:
  Graphics(const Options &options) noexcept;
  ~Graphics() noexcept;

  /** TIS->TBC conversion */
  bool tisToTBC(const std::string &inFile, const std::string &outFile) noexcept;
  /** TBC->TIS conversion */
  bool tbcToTIS(const std::string &inFile, const std::string &outFile) noexcept;
  /** MOS->MBC conversion */
  bool mosToMBC(const std::string &inFile, const std::string &outFile) noexcept;
  /** MBC->MOS conversion */
  bool mbcToMOS(const std::string &inFile, const std::string &outFile) noexcept;

  /** Read-only access to Options structure. */
  const Options& getOptions() const noexcept { return m_options; }

  /** Access to DXTn transcoder. */
  DxtPtr& getTranscoder() noexcept { return m_transcoder; }

  /**
   * Processes the given tile data, depending on its configuration.
   * Returns the processed tile data, or nullptr on error.
   */
  TileDataPtr processTile(TileDataPtr tileData) noexcept;

private:
  // Called by tisToTBC() and mosToMBC() to write an encoded tile to the output file
  bool writeEncodedTile(TileDataPtr tileData, File &file, double &ratio) noexcept;

  // Called by tbcToTIS() to write a decoded tile to the output file
  bool writeDecodedTisTile(TileDataPtr tileData, File &file) noexcept;

  /// Called by mbcToMOS() to write a decoded tile to the output file
  bool writeDecodedMosTile(TileDataPtr tileData, BytePtr mosData, uint32_t &palOfs,
                           uint32_t &tileOfs, uint32_t &dataOfsRel, uint32_t dataOfsBase) noexcept;

  // Encodes a single tile. Returns encoded tile data or nullptr on error.
  TileDataPtr encodeTile(TileDataPtr tileData) noexcept;

  // Decodes a single tile. Returns decoded tile data or nullptr on error.
  TileDataPtr decodeTile(TileDataPtr tileData) noexcept;

  // Update the progression of a progress bar. Returns updated curProgress.
  unsigned showProgress(unsigned curTile, unsigned maxTiles,
                        unsigned curProgress, unsigned maxProgress,
                        char symbol) const noexcept;

public:
  static const char HEADER_TIS_SIGNATURE[4];          // TIS signature
  static const char HEADER_MOS_SIGNATURE[4];          // MOS signature
  static const char HEADER_MOSC_SIGNATURE[4];         // MOSC signature
  static const char HEADER_TBC_SIGNATURE[4];          // TBC signature
  static const char HEADER_MBC_SIGNATURE[4];          // MBC signature
  static const char HEADER_VERSION_V1[4];             // TIS/MOS file version
  static const char HEADER_VERSION_V1_0[4];           // TBC/MBC file version

  static const unsigned HEADER_TBC_SIZE;              // TBC header size
  static const unsigned HEADER_MBC_SIZE;              // MBC header size
  static const unsigned HEADER_TILE_ENCODED_SIZE;     // header size for a raw/BCx encoded tile
  static const unsigned HEADER_TILE_COMPRESSED_SIZE;  // header size for a zlib compressed tile

private:
  static const unsigned PALETTE_SIZE;                 // palette size in bytes
  static const unsigned MAX_TILE_SIZE_8;              // max. size (in bytes) of a 8-bit pixels tile
  static const unsigned MAX_TILE_SIZE_32;             // max. size (in bytes) of a 32-bit pixels tile

  static const unsigned MAX_PROGRESS;                 // Available space for a progress bar

  const Options&  m_options;
  DxtPtr          m_transcoder;
};


#endif
