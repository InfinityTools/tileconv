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


namespace tc {

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
  /** TIZ->TIS conversion */
  bool tizToTIS(const std::string &inFile, const std::string &outFile) noexcept;
  /** TIZ->TIS conversion */
  bool mozToMOS(const std::string &inFile, const std::string &outFile) noexcept;

  /** Read-only access to Options structure. */
  const Options& getOptions() const noexcept { return m_options; }

private:
  // Read TIS header data. File points to start of tile data afterwards.
  bool readTIS(File &fin, unsigned &numTiles) noexcept;
  // Reads MOS file. mos contains uncompressed MOS data.
  bool readMOS(File &fin, BytePtr &mos, unsigned &width, unsigned &height, unsigned &palOfs) noexcept;
  // Reads TBC header data. File points to start of tile data afterwards.
  bool readTBC(File &fin, unsigned &type, unsigned &numTiles) noexcept;
  // Reads MBC header data. File points to start of tile data afterwards.
  bool readMBC(File &fin, unsigned &type, unsigned &width, unsigned &height) noexcept;
  // Reads TIZ header data. File points to start of tile data afterwards.
  bool readTIZ(File &fin, unsigned &type, unsigned &numTiles) noexcept;
  // Reads MOZ header data. File points to start of tile data afterwards.
  bool readMOZ(File &fin, unsigned &type, unsigned &width, unsigned &height) noexcept;

  // write data as MOS or MOSC to disk
  bool writeMos(File &fout, BytePtr &mos, unsigned size) noexcept;

  // Called by tisToTBC() and mosToMBC() to write an encoded tile to the output file
  bool writeEncodedTile(TileDataPtr tileData, File &file, double &ratio) noexcept;

  // Called by tbcToTIS() to write a decoded tile to the output file
  bool writeDecodedTisTile(TileDataPtr tileData, File &file) noexcept;

  /// Called by mbcToMOS() to write a decoded tile to the output file
  bool writeDecodedMosTile(TileDataPtr tileData, BytePtr mosData, uint32_t &palOfs,
                           uint32_t &tileOfs, uint32_t &dataOfsRel, uint32_t dataOfsBase) noexcept;

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

  static const char HEADER_TIZ_SIGNATURE[4];          // TIZ signature
  static const char HEADER_MOZ_SIGNATURE[4];          // MOZ signature
  static const char HEADER_TIL0_SIGNATURE[4];         // TIZ/MOZ tile v0 signature
  static const char HEADER_TIL1_SIGNATURE[4];         // TIZ/MOZ tile v1 signature
  static const char HEADER_TIL2_SIGNATURE[4];         // TIZ/MOZ tile v2 signature

  static const char HEADER_VERSION_V1[4];             // TIS/MOS file version
  static const char HEADER_VERSION_V2[4];             // TIS/MOS file version
  static const char HEADER_VERSION_V1_0[4];           // TBC/MBC file version

private:
  static const unsigned MAX_PROGRESS;                 // Available space for a progress bar
  static const unsigned MAX_POOL_TILES;               // Max. storage of tiles in thread pool

  const Options&  m_options;
};

}   // namespace tc

#endif
