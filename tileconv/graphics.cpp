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
#include <memory>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <squish.h>
#include "funcs.h"
#include "graphics.h"
#include "colors.h"
#include "compress.h"


const char Graphics::HEADER_TIS_SIGNATURE[4]  = {'T', 'I', 'S', ' '};
const char Graphics::HEADER_MOS_SIGNATURE[4]  = {'M', 'O', 'S', ' '};
const char Graphics::HEADER_MOSC_SIGNATURE[4] = {'M', 'O', 'S', 'C'};
const char Graphics::HEADER_TBC_SIGNATURE[4]  = {'T', 'B', 'C', ' '};
const char Graphics::HEADER_MBC_SIGNATURE[4]  = {'M', 'B', 'C', ' '};
const char Graphics::HEADER_VERSION_V1[4]     = {'V', '1', ' ', ' '};
const char Graphics::HEADER_VERSION_V1_0[4]   = {'V', '1', '.', '0'};

const unsigned Graphics::HEADER_TBC_SIZE             = 16;
const unsigned Graphics::HEADER_MBC_SIZE             = 20;
const unsigned Graphics::HEADER_TILE_ENCODED_SIZE    = 4;
const unsigned Graphics::HEADER_TILE_COMPRESSED_SIZE = 4;

const unsigned Graphics::PALETTE_SIZE                = 1024;
const unsigned Graphics::MAX_TILE_SIZE_8             = 64*64;
const unsigned Graphics::MAX_TILE_SIZE_32            = 64*64*4;


Graphics::Graphics(const Options &options) noexcept
: m_options(options)
{
}

Graphics::~Graphics() noexcept
{
}

bool Graphics::tisToTBC(const std::string &inFile, const std::string &outFile) noexcept
{
  if (!inFile.empty() && !outFile.empty() && inFile != outFile) {
    File fin(inFile.c_str(), "rb");
    if (!fin.error()) {
      char sig[4], ver[4];
      uint32_t tileCount, tileSize, headerSize, tileDim;

      // parsing TIS header
      if (fin.read(sig, 1, 4) != 4) return false;;
      if (std::strncmp(sig, HEADER_TIS_SIGNATURE, 4) != 0) {
        std::printf("Invalid TIS signature\n");
        return false;
      }

      if (fin.read(ver, 1, 4) != 4) return false;
      if (std::strncmp(ver, HEADER_VERSION_V1, 4) != 0) {
        std::printf("Invalid TIS version\n");
        return false;
      }

      if (fin.read(&tileCount, 4, 1) != 1) return false;
      tileCount = get32u(&tileCount);
      if (tileCount == 0) {
        std::printf("No tiles found\n");
        return false;
      }

      if (fin.read(&tileSize, 4, 1) != 1) return false;
      tileSize = get32u(&tileSize);
      if (tileSize != 0x1400) {
        if (tileSize == 0x000c) {
          std::printf("PVRZ-based TIS files are not supported\n");
        } else {
          std::printf("Invalid tile size\n");
        }
        return false;
      }

      if (fin.read(&headerSize, 4, 1) != 1) return false;
      headerSize = get32u(&headerSize);
      if (headerSize < 0x18) {
        std::printf("Invalid header size\n");
        return false;
      }

      if (fin.read(&tileDim, 4, 1) != 1) return false;
      tileDim = get32u(&tileDim);
      if (tileDim != 0x40) {
        std::printf("Invalid tile dimensions\n");
        return false;
      }

      File fout(outFile.c_str(), "wb");
      if (!fout.error()) {
        uint32_t v32;
        uint32_t tileSizeIndexed = tileDim*tileDim;   // indexed size of the tile

        // writing TBC header
        if (fout.write(HEADER_TBC_SIGNATURE, 1, sizeof(HEADER_TBC_SIGNATURE)) != sizeof(HEADER_TBC_SIGNATURE)) return false;
        if (fout.write(HEADER_VERSION_V1_0, 1, sizeof(HEADER_VERSION_V1_0)) != sizeof(HEADER_VERSION_V1_0)) return false;
        v32 = Options::GetEncodingCode(getOptions().getEncoding(), getOptions().isDeflate());
        v32 = get32u(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing encoding type
        v32 = get32u(&tileCount);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile count
        if (!getOptions().isSilent()) std::printf("Tile count: %d\n", tileCount);

        // converting tiles
        BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
        BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
        BytePtr ptrDeflated(new uint8_t[MAX_TILE_SIZE_32*2], std::default_delete<uint8_t[]>());
        double ratioCount = 0.0;    // counts the compression ratios of all tiles
        for (unsigned tileIdx = 0; tileIdx < tileCount; tileIdx++) {
          if (getOptions().isVerbose()) std::printf("Converting tile #%d\n", tileIdx);

          // creating new tile data object
          TileDataPtr tileData(new TileData(tileIdx, ptrIndexed, ptrPalette, ptrDeflated,
                                            tileDim, tileDim, 0, 0));
          // reading paletted tile
          if (fin.read(tileData->ptrPalette.get(), 1, PALETTE_SIZE) != PALETTE_SIZE) {
            return false;
          }
          if (fin.read(tileData->ptrIndexed.get(), 1, tileSizeIndexed) != tileSizeIndexed) {
            return false;
          }

          // encoding tile
          if (!encodeTile(tileData)) {
            if (!tileData->errorMsg.empty()) {
              std::printf("%s", tileData->errorMsg.c_str());
            }
            return false;
          }

          // writing result to output file
          double ratio = 0.0;
          if (!writeEncodedTile(tileData, fout, ratio)) {
            return false;
          }
          ratioCount += ratio;
        }

        // displaying summary
        if (!getOptions().isSilent()) {
          std::printf("TIS file converted successfully. Total compression ratio: %.2f%%.\n",
                      ratioCount / (double)tileCount);
        }

        return true;
      }
    } else {
      std::printf("Error opening file \"%s\"\n", inFile.c_str());
    }
  }
  return false;
}


bool Graphics::tbcToTIS(const std::string &inFile, const std::string &outFile) noexcept
{
  if (!inFile.empty() && !outFile.empty() && inFile != outFile) {
    File fin(inFile.c_str(), "rb");
    if (!fin.error()) {
      char sig[4], ver[4];
      uint32_t compType, tileCount;

      // parsing TBC header
      if (fin.read(sig, 1, 4) != 4) return false;;
      if (std::strncmp(sig, HEADER_TBC_SIGNATURE, 4) != 0) {
        std::printf("Invalid TBC signature\n");
        return false;
      }

      if (fin.read(ver, 1, 4) != 4) return false;
      if (std::strncmp(ver, HEADER_VERSION_V1_0, 4) != 0)
        std::printf("Unsupported TBC version\n");
        return false;

      if (fin.read(&compType, 4, 1) != 1) return false;
      compType = get32u(&compType);

      if (fin.read(&tileCount, 4, 1) != 1) return false;
      tileCount = get32u(&tileCount);
      if (tileCount == 0) {
        std::printf("No tiles found\n");
        return false;
      }

      File fout(outFile.c_str(), "wb");
      if (!fout.error()) {
        uint32_t v32;

        // writing TIS header
        if (fout.write(HEADER_TIS_SIGNATURE, 1, sizeof(HEADER_TIS_SIGNATURE)) != sizeof(HEADER_TIS_SIGNATURE)) return false;
        if (fout.write(HEADER_VERSION_V1, 1, sizeof(HEADER_VERSION_V1)) != sizeof(HEADER_VERSION_V1)) return false;
        v32 = get32u(&tileCount);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile count
        v32 = 0x1400; v32 = get32u(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile size
        v32 = 0x18; v32 = get32u(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing header size
        v32 = 0x40; v32 = get32u(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile dimension

        if (getOptions().isVerbose()) {
          std::printf("Tile count: %d, encoding: %d - %s\n",
                      tileCount, compType, Options::GetEncodingName(compType).c_str());
        }

        BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
        BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
        BytePtr ptrDeflated(new uint8_t[MAX_TILE_SIZE_32*2], std::default_delete<uint8_t[]>());
        for (uint32_t tileIdx = 0; tileIdx < tileCount; tileIdx++) {
          // creating new tile data object
          uint32_t chunkSize;
          if (fin.read(&chunkSize, 4, 1) != 1) return false;
          chunkSize = get32u(&chunkSize);
          if (chunkSize == 0) {
            std::printf("Invalid block size found for tile #%d\n", tileIdx);
            return false;
          }
          TileDataPtr tileData(new TileData(tileIdx, ptrIndexed, ptrPalette, ptrDeflated,
                                            0, 0, compType, chunkSize));
          if (fin.read(tileData->ptrDeflated.get(), 1, chunkSize) != chunkSize) {
            return false;
          }

          if (!decodeTile(tileData)) {
            if (!tileData->errorMsg.empty()) {
              std::printf("%s", tileData->errorMsg.c_str());
            }
            return false;
          }

          if (!writeDecodedTisTile(tileData, fout)) {
            return false;
          }
        }

        // displaying summary
        if (!getOptions().isSilent()) {
          std::printf("TBC file converted successfully.\n");
        }

        return true;
      }
    } else {
      std::printf("Error opening file \"%s\"\n", inFile.c_str());
    }
  }
  return false;
}


bool Graphics::mosToMBC(const std::string &inFile, const std::string &outFile) noexcept
{
  if (!inFile.empty() && !outFile.empty() && inFile != outFile) {
    File fin(inFile.c_str(), "rb");
    if (!fin.error()) {
      char sig[4], ver[4];
      uint32_t mosSize, tileDim, palOfs;
      uint16_t mosWidth, mosHeight, mosCols, mosRows;
      BytePtr mosData(nullptr, std::default_delete<uint8_t[]>());

      // loading MOS/MOSC input file
      if (fin.read(sig, 1, 4) != 4) return false;;
      if (std::strncmp(sig, HEADER_MOSC_SIGNATURE, 4) == 0) {    // decompressing MOSC
        uint32_t moscSize;
        Compression compression;

        // getting MOSC file size
        fin.seek(0, SEEK_END);
        moscSize = fin.tell();
        if (moscSize <= 12) {
          std::printf("Invalid MOSC size\n");
          return false;
        }
        moscSize -= 12;    // removing header size

        fin.seek(4, SEEK_SET);
        if (fin.read(&ver, 1, 4) != 4) return false;
        if (std::strncmp(ver, HEADER_VERSION_V1, 4) != 0) {
          std::printf("Invalid MOSC version\n");
          return false;
        }

        if (fin.read(&mosSize, 4, 1) != 1) return false;;
        mosSize = get32u(&mosSize);
        if (mosSize < 24) {
          std::printf("MOS size too small\n");
          return false;
        }
        BytePtr moscData(new uint8_t[moscSize], std::default_delete<uint8_t[]>());
        if (fin.read(moscData.get(), 1, moscSize) < moscSize) {
          std::printf("Incomplete or corrupted MOSC file\n");
          return false;
        }

        mosData.reset(new uint8_t[mosSize], std::default_delete<uint8_t[]>());
        uint32_t size = compression.inflate(moscData.get(), moscSize, mosData.get(), mosSize);
        if (size != mosSize) {
          std::printf("Error while decompressing MOSC input file\n");
          return false;
        }
      } else if (std::strncmp(sig, HEADER_MOS_SIGNATURE, 4) == 0) {   // loading MOS data
        fin.seek(0, SEEK_END);
        mosSize = fin.tell();
        if (mosSize < 24) {
          std::printf("MOS size too small\n");
          return false;
        }
        fin.seek(0, SEEK_SET);
        mosData.reset(new uint8_t[mosSize], std::default_delete<uint8_t[]>());
        if (fin.read(mosData.get(), 1, mosSize) != mosSize) return false;
      } else {
        std::printf("Invalid MOS signature\n");
        return false;
      }

      // parsing MOS header
      uint32_t inOfs = 0;
      if (std::memcmp(mosData.get()+inOfs, HEADER_MOS_SIGNATURE, 4) != 0) {
        std::printf("Invalid MOS signature\n");
        return false;
      }
      inOfs += 4;

      if (std::memcmp(mosData.get()+inOfs, HEADER_VERSION_V1, 4) != 0) {
        std::printf("Unsupported MOS version\n");
        return false;
      }
      inOfs += 4;

      std::memcpy(&mosWidth, mosData.get()+inOfs, 2);
      mosWidth = get16u(&mosWidth);
      if (mosWidth == 0) {
        std::printf("Invalid MOS width\n");
        return false;
      }
      inOfs += 2;

      mosHeight = get16u((uint16_t*)(mosData.get()+inOfs));
      if (mosHeight == 0) {
        std::printf("Invalid MOS height\n");
        return false;
      }
      inOfs += 2;

      mosCols = get16u((uint16_t*)(mosData.get()+inOfs));
      if (mosCols == 0) {
        std::printf("Invalid number of tiles\n");
        return false;
      }
      inOfs += 2;

      mosRows = get16u((uint16_t*)(mosData.get()+inOfs));
      if (mosRows == 0) {
        std::printf("Invalid number of tiles\n");
        return false;
      }
      inOfs += 2;

      tileDim = get32u((uint32_t*)(mosData.get()+inOfs));
      if (tileDim != 0x40) {    // TODO: expand MBC header to allow non-hardcoded tile dimensions
        std::printf("Invalid tile dimensions\n");
        return false;
      }
      inOfs += 4;

      palOfs = get32u((uint32_t*)(mosData.get()+inOfs));
      if (palOfs < 24) {
        std::printf("MOS header too small\n");
        return false;
      }
      inOfs = palOfs;

      {
        // comparing calculated size with actual input file length
        uint32_t size = palOfs + mosCols*mosRows*PALETTE_SIZE + mosCols*mosRows*4 + mosWidth*mosHeight;
        if (mosSize < size) {
          std::printf("Incomplete or corrupted MOS file\n");
          return false;
        }
      }

      File fout(outFile.c_str(), "wb");
      if (!fout.error()) {
        uint32_t v32;
        uint32_t tileCount = mosCols * mosRows;
        uint32_t tileOfs = palOfs + tileCount*PALETTE_SIZE;   // start offset of dword array with relative offsets into tile data
        uint32_t dataOfs = tileOfs + tileCount*4;             // start offset of tile data

        // writing MBC header
        if (fout.write(HEADER_MBC_SIGNATURE, 1, sizeof(HEADER_MBC_SIGNATURE)) != sizeof(HEADER_MBC_SIGNATURE)) return false;
        if (fout.write(HEADER_VERSION_V1_0, 1, sizeof(HEADER_VERSION_V1_0)) != sizeof(HEADER_VERSION_V1_0)) return false;
        v32 = Options::GetEncodingCode(getOptions().getEncoding(), getOptions().isDeflate());
        v32 = get32u(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing encoding type
        v32 = mosWidth; v32 = get32u(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing MOS width
        v32 = mosHeight; v32 = get32u(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing MOS height
        if (getOptions().isVerbose()) std::printf("Tile count: %d\n", tileCount);

        // processing tiles
        BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
        BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
        BytePtr ptrDeflated(new uint8_t[MAX_TILE_SIZE_32*2], std::default_delete<uint8_t[]>());
        double ratioCount = 0.0;              // counts the compression ratios of all tiles
        int curIndex = 0;
        uint32_t remTileHeight = mosHeight;   // remaining tile height to cover
        for (uint32_t row = 0; row < mosRows; row++, remTileHeight -= tileDim) {
          uint32_t tileHeight = std::min(tileDim, remTileHeight);
          uint32_t remTileWidth = mosWidth;   // remaining tile width to cover
          for (uint32_t col = 0; col < mosCols; col++, curIndex++, remTileWidth -= tileDim) {
            uint32_t tileWidth = std::min(tileDim, remTileWidth);
            uint32_t tileSizeIndexed = tileWidth*tileHeight;

            if (getOptions().isVerbose()) std::printf("Converting tile #%d\n", curIndex);

            // creating new tile data object
            TileDataPtr tileData(new TileData(curIndex, ptrIndexed, ptrPalette, ptrDeflated,
                                              tileWidth, tileHeight, 0, 0));
            // reading paletted tile
            std::memcpy(tileData->ptrPalette.get(), mosData.get()+palOfs, PALETTE_SIZE);
            palOfs += PALETTE_SIZE;
            // reading tile data
            std::memcpy(&v32, mosData.get()+tileOfs, 4);
            v32 = get32u(&v32);
            tileOfs += 4;
            std::memcpy(tileData->ptrIndexed.get(), mosData.get()+dataOfs+v32, tileSizeIndexed);

            if (!encodeTile(tileData)) {
              if (!tileData->errorMsg.empty()) {
                std::printf("%s", tileData->errorMsg.c_str());
              }
              return false;
            }

            double ratio = 0.0;
            if (!writeEncodedTile(tileData, fout, ratio)) {
              return false;
            }
            ratioCount += ratio;
          }
        }

        // displaying summary
        if (!getOptions().isSilent()) {
          std::printf("MOS file converted successfully. Total compression ratio: %.2f%%.\n",
                      ratioCount / (double)tileCount);
        }

        return true;
      }
    } else {
      std::printf("Error opening file \"%s\"\n", inFile.c_str());
    }
  }
  return false;
}


bool Graphics::mbcToMOS(const std::string &inFile, const std::string &outFile) noexcept
{
  if (!inFile.empty() && !outFile.empty() && inFile != outFile) {
    File fin(inFile.c_str(), "rb");
    if (!fin.error()) {
      char sig[4], ver[4];
      uint32_t compType, mosWidth, mosHeight;

      // parsing TBC header
      if (fin.read(sig, 1, 4) != 4) return false;;
      if (std::strncmp(sig, HEADER_MBC_SIGNATURE, 4) != 0) {
        std::printf("Invalid MBC signature\n");
        return false;
      }

      if (fin.read(ver, 1, 4) != 4) return false;
      if (std::strncmp(ver, HEADER_VERSION_V1_0, 4) != 0) {
        std::printf("Invalid MBC version\n");
        return false;
      }

      if (fin.read(&compType, 4, 1) != 1) return false;
      compType = get32u(&compType);

      if (fin.read(&mosWidth, 4, 1) != 1) return false;
      mosWidth = get32u(&mosWidth);
      if (mosWidth == 0) {
        std::printf("Invalid MBC width\n");
        return false;
      }

      if (fin.read(&mosHeight, 4, 1) != 1) return false;
      mosHeight = get32u(&mosHeight);
      if (mosHeight == 0) {
        std::printf("Invalid MBC height\n");
        return false;
      }

      File fout(outFile.c_str(), "wb");
      if (!fout.error()) {
        uint16_t v16;
        uint32_t v32;

        // creating a memory mapped copy of the output file
        uint32_t mosCols = (mosWidth + 63) / 64;
        uint32_t mosRows = (mosHeight + 63) / 64;
        uint32_t palOfs = 0x18;                                             // offset to palette data
        uint32_t tileOfs = palOfs + mosCols*mosRows*PALETTE_SIZE;           // offset to tile offset array
        uint32_t dataOfsBase = tileOfs + mosCols*mosRows*4, dataOfsRel = 0;     // abs. and rel. offsets to data blocks
        uint32_t mosSize = dataOfsBase + mosWidth*mosHeight;
        BytePtr mosData(new uint8_t[mosSize], std::default_delete<uint8_t[]>());

        // writing MOS header
        uint32_t headerOfs = 0;
        std::memcpy(mosData.get()+headerOfs, HEADER_MOS_SIGNATURE, sizeof(HEADER_MOS_SIGNATURE));
        headerOfs += 4;
        std::memcpy(mosData.get()+headerOfs, HEADER_VERSION_V1, sizeof(HEADER_VERSION_V1));
        headerOfs += 4;
        v16 = mosWidth; v16 = get16u(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos width
        headerOfs += 2;
        v16 = mosHeight; v16 = get16u(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos height
        headerOfs += 2;
        v16 = mosCols; v16 = get16u(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos columns
        headerOfs += 2;
        v16 = mosRows; v16 = get16u(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos rows
        headerOfs += 2;
        v32 = 0x40; v32 = get32u(&v32);
        std::memcpy(mosData.get()+headerOfs, &v32, 4);     // writing tile dimension
        headerOfs += 4;
        v32 = 0x18; v32 = get32u(&v32);
        std::memcpy(mosData.get()+headerOfs, &v32, 4);     // writing offset to palettes
        headerOfs += 4;

        if (getOptions().isVerbose()) {
          std::printf("Width: %d, height: %d, columns: %d, rows: %d, encoding: %d - %s\n",
                      mosWidth, mosHeight, mosCols, mosRows, compType, Options::GetEncodingName(compType).c_str());
        }

        BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
        BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
        BytePtr ptrDeflated(new uint8_t[MAX_TILE_SIZE_32*2], std::default_delete<uint8_t[]>());
        int curIndex = 0;                       // the current tile index
        for (uint32_t row = 0; row < mosRows; row++) {
          for (uint32_t col = 0; col < mosCols; col++, curIndex++) {
            if (getOptions().isVerbose()) std::printf("Decoding tile #%d\n", curIndex);

            // creating new tile data object
            uint32_t chunkSize;
            if (fin.read(&chunkSize, 4, 1) != 1) return false;
            chunkSize = get32u(&chunkSize);
            if (chunkSize == 0) {
              std::printf("Invalid block size found for tile #%d\n", curIndex);
              return false;
            }
            TileDataPtr tileData(new TileData(curIndex, ptrIndexed, ptrPalette, ptrDeflated,
                                              0, 0, compType, chunkSize));
            if (fin.read(tileData->ptrDeflated.get(), 1, chunkSize) != chunkSize) {
              return false;
            }

            if (!decodeTile(tileData)) {
              if (!tileData->errorMsg.empty()) {
                std::printf("%s", tileData->errorMsg.c_str());
              }
              return false;
            }

            if (!writeDecodedMosTile(tileData, mosData, palOfs, tileOfs, dataOfsRel, dataOfsBase)) {
              return false;
            }
          }
        }

        if (getOptions().isMosc()) {
          // compressing mos -> mosc
          uint32_t moscSize = mosSize*2;
          Compression compression;
          BytePtr moscData(new uint8_t[moscSize], std::default_delete<uint8_t[]>());

          // writing MOSC header
          std::memcpy(moscData.get(), HEADER_MOSC_SIGNATURE, sizeof(HEADER_MOSC_SIGNATURE));
          std::memcpy(moscData.get()+4, HEADER_VERSION_V1, sizeof(HEADER_VERSION_V1));
          v32 = mosSize; v32 = get32u(&v32);
          std::memcpy(moscData.get()+8, &v32, 4);

          // compressing data
          uint32_t size = compression.deflate(mosData.get(), mosSize, moscData.get()+12, moscSize-12);
          moscSize = size + 12;

          // writing cmos data to file
          if (fout.write(moscData.get(), 1, moscSize) != moscSize) return false;
        } else {
          // writing mos data to file
          if (fout.write(mosData.get(), 1, mosSize) != mosSize) return false;
        }

        // displaying summary
        if (!getOptions().isSilent()) {
          std::printf("MBC file converted successfully.\n");
        }

        return true;
      }
    } else {
      std::printf("Error opening file \"%s\"\n", inFile.c_str());
    }
  }
  return false;
}


bool Graphics::writeEncodedTile(TileDataPtr tileData, File &file, double &ratio) noexcept
{
  if (tileData != nullptr) {
    if (tileData->size > 0) {
      uint32_t v32 = tileData->size; v32 = get32u(&v32);  // compressed tile size in ready-to-write format
      if (file.write(&v32, 4, 1) != 1) {
        std::printf("Error while writing tile data\n");
        return false;
      }
      if (file.write(tileData->ptrDeflated.get(), 1, tileData->size) != tileData->size) {
        std::printf("Error while writing tile data\n");
        return false;
      }

      // displaying statistical information
      int tileSizeIndexed = tileData->tileWidth * tileData->tileHeight;
      ratio = ((double)(tileData->size+HEADER_TILE_COMPRESSED_SIZE)*100.0) / (double)(tileSizeIndexed+PALETTE_SIZE);
      if (getOptions().isVerbose()) {
        std::printf("Conversion finished. Original size = %d bytes. Compressed size = %d bytes. Compression ratio: %.2f%%.\n",
                    tileSizeIndexed+PALETTE_SIZE, tileData->size+HEADER_TILE_COMPRESSED_SIZE, ratio);
      }
      return true;
    } else {
      std::printf("%s", tileData->errorMsg.c_str());
    }
  }
  return false;
}


bool Graphics::writeDecodedTisTile(TileDataPtr tileData, File &file) noexcept
{
  if (tileData != nullptr) {
    if (tileData->size > 0) {
      if (file.write(tileData->ptrPalette.get(), 1, PALETTE_SIZE) != PALETTE_SIZE) {
        std::printf("Error while writing tile data\n");
        return false;
      }
      if (file.write(tileData->ptrIndexed.get(), 1, MAX_TILE_SIZE_8) != MAX_TILE_SIZE_8) {
        std::printf("Error while writing tile data\n");
        return false;
      }

      if (getOptions().isVerbose()) {
        std::printf("Tile #%d decoded successfully\n", tileData->index);
      }
      return true;
    } else {
      std::printf("%s", tileData->errorMsg.c_str());
    }
  }
  return false;
}


bool Graphics::writeDecodedMosTile(TileDataPtr tileData, BytePtr mosData, uint32_t &palOfs,
                                   uint32_t &tileOfs, uint32_t &dataOfsRel, uint32_t dataOfsBase) noexcept
{
  if (tileData != nullptr && mosData != nullptr) {
    if (tileData->size > 0) {
      uint32_t tileSizeIndexed = tileData->tileWidth * tileData->tileHeight;

      // writing palette data
      std::memcpy(mosData.get()+palOfs, tileData->ptrPalette.get(), PALETTE_SIZE);
      palOfs += PALETTE_SIZE;

      // writing tile offsets
      uint32_t v32 = dataOfsRel; v32 = get32u(&v32);
      std::memcpy(mosData.get()+tileOfs, &v32, 4);
      tileOfs += 4;

      // writing tile data
      std::memcpy(mosData.get()+dataOfsBase+dataOfsRel, tileData->ptrIndexed.get(), tileSizeIndexed);
      dataOfsRel += tileSizeIndexed;

      if (getOptions().isVerbose()) {
        std::printf("Tile #%d decoded successfully\n", tileData->index);
      }
      return true;
    } else {
      std::printf("%s", tileData->errorMsg.c_str());
    }
  }
  return false;
}


bool Graphics::encodeTile(TileDataPtr tileData) noexcept
{
  if (tileData->ptrIndexed != nullptr && tileData->ptrPalette != nullptr &&
      tileData->ptrDeflated != nullptr && tileData->index >= 0 &&
      tileData->tileWidth > 0 && tileData->tileHeight > 0) {
    BytePtr  ptrEncoded(new uint8_t[MAX_TILE_SIZE_32], std::default_delete<uint8_t[]>());
    uint16_t v16;                   // temp. variable (16-bit)
    uint32_t tileWidthPadded;       // tile width expanded to a multiple of 4
    uint32_t tileHeightPadded;      // tile height expanded to a multiple of 4
    uint32_t tileSizeEncoded;       // pixel encoded size of the tile
    int      squishFlags = squish::kColourIterativeClusterFit;
    Colors   colors(getOptions());
    Compression compression;

    tileData->size = 0;

    switch (getOptions().getEncoding()) {
      case Encoding::RAW:
        tileWidthPadded = tileData->tileWidth;
        tileHeightPadded = tileData->tileHeight;
        tileSizeEncoded = tileWidthPadded*tileHeightPadded + PALETTE_SIZE;
        break;
      case Encoding::BC2:
        tileWidthPadded = colors.getPaddedValue(tileData->tileWidth);
        tileHeightPadded = colors.getPaddedValue(tileData->tileHeight);
        squishFlags |= squish::kDxt3;
        tileSizeEncoded = squish::GetStorageRequirements(tileWidthPadded, tileHeightPadded, squishFlags);
        break;
      case Encoding::BC3:
        tileWidthPadded = colors.getPaddedValue(tileData->tileWidth);
        tileHeightPadded = colors.getPaddedValue(tileData->tileHeight);
        squishFlags |= squish::kDxt5;
        tileSizeEncoded = squish::GetStorageRequirements(tileWidthPadded, tileHeightPadded, squishFlags);
        break;
      default:    // default is BC1
        tileWidthPadded = colors.getPaddedValue(tileData->tileWidth);
        tileHeightPadded = colors.getPaddedValue(tileData->tileHeight);
        squishFlags |= squish::kDxt1;
        tileSizeEncoded = squish::GetStorageRequirements(tileWidthPadded, tileHeightPadded, squishFlags);
        break;
    }
    uint32_t tileSizePixels = tileData->tileWidth * tileData->tileHeight;
    uint32_t tileSizePixelsPadded = tileWidthPadded*tileHeightPadded;

    // padding tile (if needed) and applying pixel encodings
    switch (getOptions().getEncoding()) {
      case Encoding::RAW:
      {
        // setting tile header
        uint8_t *encodedPtr = ptrEncoded.get();
        v16 = (uint16_t)tileData->tileWidth; v16 = get16u(&v16);    // tile width in ready-to-write format
        std::memcpy(encodedPtr, &v16, 2); encodedPtr += 2;          // setting tile width
        v16 = (uint16_t)tileData->tileHeight; v16 = get16u(&v16);   // tile height in ready-to-write format
        std::memcpy(encodedPtr, &v16, 2); encodedPtr += 2;          // setting tile height
        // copying palette and pixel data
        std::memcpy(encodedPtr, tileData->ptrPalette.get(), PALETTE_SIZE);
        encodedPtr += PALETTE_SIZE;
        std::memcpy(encodedPtr, tileData->ptrIndexed.get(), tileSizePixels);
        encodedPtr += tileSizePixels;
        break;
      }
      case Encoding::BC1:
      case Encoding::BC2:
      case Encoding::BC3:
      {
        BytePtr ptrARGB(new uint8_t[MAX_TILE_SIZE_32], std::default_delete<uint8_t[]>());
        BytePtr ptrARGB2(new uint8_t[MAX_TILE_SIZE_32], std::default_delete<uint8_t[]>());

        // converting tile to ARGB
        if (colors.palToARGB(tileData->ptrIndexed.get(), tileData->ptrPalette.get(),
                             ptrARGB.get(), tileSizePixels) != tileSizePixels) {
          tileData->errorMsg.assign("Error during color space conversion\n");
          return false;
        }
        // padding tile if needed
        if (colors.padBlock(ptrARGB.get(), ptrARGB2.get(), tileData->tileWidth, tileData->tileHeight,
                            tileWidthPadded, tileHeightPadded) != tileSizePixelsPadded) {
          tileData->errorMsg.assign("Error while encoding pixels\n");
          return false;
        }
        // preparing correct color components order
        if (colors.reorderColors(ptrARGB2.get(), tileSizePixelsPadded,
                                 Colors::FMT_ARGB, Colors::FMT_ABGR) < tileSizePixelsPadded) {
          tileData->errorMsg.assign("Error while encoding pixels\n");
          return false;
        }
        uint8_t *argbPtr = ptrARGB2.get();
        uint8_t *encodedPtr = ptrEncoded.get();
        // setting tile header
        v16 = (uint16_t)tileData->tileWidth; v16 = get16u(&v16);  // tile width in ready-to-write format
        std::memcpy(encodedPtr, &v16, 2); encodedPtr += 2;        // setting tile width
        v16 = (uint16_t)tileData->tileHeight; v16 = get16u(&v16); // tile height in ready-to-write format
        std::memcpy(encodedPtr, &v16, 2); encodedPtr += 2;        // setting tile height
        // encoding pixel data
        squish::CompressImage(argbPtr, tileWidthPadded, tileHeightPadded, encodedPtr, squishFlags);
        break;
      }
    }

    if (getOptions().isDeflate()) {
      // applying zlib compression
      tileData->size = compression.deflate(ptrEncoded.get(), tileSizeEncoded+HEADER_TILE_ENCODED_SIZE,
                                           tileData->ptrDeflated.get(), MAX_TILE_SIZE_32*2);
      if (tileData->size == 0) {
        tileData->errorMsg.assign("Error while compressing tile data\n");
        return false;
      }
    } else {
      // using pixel encoding only
      tileData->size = tileSizeEncoded+HEADER_TILE_ENCODED_SIZE;
      std::memcpy(tileData->ptrDeflated.get(), ptrEncoded.get(), tileData->size);
    }

    return true;
  }
  return false;
}


bool Graphics::decodeTile(TileDataPtr tileData) noexcept
{
  if (tileData->ptrIndexed != nullptr && tileData->ptrPalette != nullptr &&
      tileData->ptrDeflated != nullptr && tileData->index >= 0 && tileData->size > 0) {
    BytePtr  ptrARGB(new uint8_t[MAX_TILE_SIZE_32], std::default_delete<uint8_t[]>());
    BytePtr  ptrARGB2(new uint8_t[MAX_TILE_SIZE_32], std::default_delete<uint8_t[]>());
    BytePtr  ptrEncoded(new uint8_t[MAX_TILE_SIZE_32], std::default_delete<uint8_t[]>());
    uint16_t v16;
    uint32_t tileWidth, tileHeight;
    uint32_t tileSizeIndexed;
    Colors   colors(getOptions());
    Compression compression;

    if (Options::IsTileDeflated(tileData->encodingType)) {
      // inflating zlib compressed data
      compression.inflate(tileData->ptrDeflated.get(), tileData->size, ptrEncoded.get(), MAX_TILE_SIZE_32);
    } else {
      // copy pixel encoded tile data
      std::memcpy(ptrEncoded.get(), tileData->ptrDeflated.get(), tileData->size);
    }

    tileData->size = 0;

    // retrieving tile dimensions
    std::memcpy(&v16, ptrEncoded.get(), 2);     // reading tile width
    v16 = get16u(&v16);
    tileWidth = v16;
    tileData->tileWidth = tileWidth;
    std::memcpy(&v16, ptrEncoded.get()+2, 2);   // reading tile height
    v16 = get16u(&v16);
    tileHeight = v16;
    tileData->tileHeight = tileHeight;
    tileSizeIndexed = tileWidth*tileHeight;

    uint32_t tileWidthPadded = colors.getPaddedValue(tileWidth);
    uint32_t tileHeightPadded = colors.getPaddedValue(tileHeight);

    // decoding pixel compression
    if (Options::GetEncodingType(tileData->encodingType) == Encoding::RAW) {
      // simply extracting palette and pixel data
      std::memcpy(tileData->ptrPalette.get(), ptrEncoded.get()+HEADER_TILE_ENCODED_SIZE, PALETTE_SIZE);
      std::memcpy(tileData->ptrIndexed.get(), ptrEncoded.get()+HEADER_TILE_ENCODED_SIZE+PALETTE_SIZE,
                  tileSizeIndexed);
    } else {
      // decoding BCx pixel data
      int squishFlags;
      switch (Options::GetEncodingType(tileData->encodingType)) {
        case Encoding::BC2: squishFlags = squish::kDxt3; break;
        case Encoding::BC3: squishFlags = squish::kDxt5; break;
        default:            squishFlags = squish::kDxt1; break;
      }
      // decoding pixel data
      squish::DecompressImage(ptrARGB2.get(), tileWidthPadded, tileHeightPadded,
                              ptrEncoded.get()+HEADER_TILE_ENCODED_SIZE, squishFlags);
      // fixing color orders
      uint32_t tileSizeARGB = tileWidthPadded*tileHeightPadded;
      if (colors.reorderColors(ptrARGB2.get(), tileSizeARGB,
                               Colors::FMT_ABGR, Colors::FMT_ARGB) < tileSizeARGB) {
        tileData->errorMsg.assign("Error while decoding pixels\n");
        return false;
      }
      // unpadding tile block
      if (colors.unpadBlock(ptrARGB2.get(), ptrARGB.get(), tileWidthPadded,
                            tileHeightPadded, tileWidth, tileHeight) != tileSizeIndexed) {
        tileData->errorMsg.assign("Error while decoding pixels\n");
        return false;
      }

      // applying color reduction
      if (colors.ARGBToPal(ptrARGB.get(), tileData->ptrIndexed.get(), tileData->ptrPalette.get(),
                           tileWidth, tileHeight) != tileSizeIndexed) {
        tileData->errorMsg.assign("Error while decoding pixels\n");
        return false;
      }
    }

    tileData->size = tileSizeIndexed + PALETTE_SIZE;

    return true;
  }
  return false;
}
