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
#include "funcs.h"
#include "colors.h"
#include "compress.h"
#include "tilethreadpool.h"
#include "graphics.h"

namespace tc {

const char Graphics::HEADER_TIS_SIGNATURE[4]  = {'T', 'I', 'S', ' '};
const char Graphics::HEADER_MOS_SIGNATURE[4]  = {'M', 'O', 'S', ' '};
const char Graphics::HEADER_MOSC_SIGNATURE[4] = {'M', 'O', 'S', 'C'};

const char Graphics::HEADER_TBC_SIGNATURE[4]  = {'T', 'B', 'C', ' '};
const char Graphics::HEADER_MBC_SIGNATURE[4]  = {'M', 'B', 'C', ' '};

const char Graphics::HEADER_TIZ_SIGNATURE[4]  = {'T', 'I', 'Z', '0'};
const char Graphics::HEADER_MOZ_SIGNATURE[4]  = {'M', 'O', 'Z', '0'};
const char Graphics::HEADER_TIL0_SIGNATURE[4] = {'T', 'I', 'L', '0'};
const char Graphics::HEADER_TIL1_SIGNATURE[4] = {'T', 'I', 'L', '1'};
const char Graphics::HEADER_TIL2_SIGNATURE[4] = {'T', 'I', 'L', '2'};

const char Graphics::HEADER_VERSION_V1[4]     = {'V', '1', ' ', ' '};
const char Graphics::HEADER_VERSION_V2[4]     = {'V', '2', ' ', ' '};
const char Graphics::HEADER_VERSION_V1_0[4]   = {'V', '1', '.', '0'};

const unsigned Graphics::MAX_PROGRESS         = 69;
const unsigned Graphics::MAX_POOL_TILES       = 64;


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
      uint32_t tileCount;

      // parsing TIS header
      if (!readTIS(fin, tileCount)) return false;

      File fout(outFile.c_str(), "wb");
      fout.setDeleteOnClose(true);
      if (!fout.error()) {
        uint32_t v32;
        uint32_t tileSizeIndexed = TILE_DIMENSION*TILE_DIMENSION;   // indexed size of the tile

        // writing TBC header
        if (fout.write(HEADER_TBC_SIGNATURE, 1, sizeof(HEADER_TBC_SIGNATURE)) != sizeof(HEADER_TBC_SIGNATURE)) return false;
        if (fout.write(HEADER_VERSION_V1_0, 1, sizeof(HEADER_VERSION_V1_0)) != sizeof(HEADER_VERSION_V1_0)) return false;
        v32 = Options::GetEncodingCode(getOptions().getEncoding(), getOptions().isDeflate());
        v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing encoding type
        v32 = get32u_le(&tileCount);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile count
        if (!getOptions().isSilent()) std::printf("Tile count: %d\n", tileCount);
        if (getOptions().getVerbosity() == 1) std::printf("Converting");

        // converting tiles
        ThreadPoolPtr pool = createThreadPool(getOptions().getThreads(), MAX_POOL_TILES);
        double ratioCount = 0.0;    // counts the compression ratios of all tiles
        unsigned tileIdx = 0, nextTileIdx = 0, curProgress = 0;
        while (tileIdx < tileCount || !pool->finished()) {
          // creating new tile data object
          if (tileIdx < tileCount) {
            if (getOptions().isVerbose()) std::printf("Converting tile #%d\n", tileIdx);
            BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
            BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
            BytePtr ptrDeflated(new uint8_t[MAX_TILE_SIZE_32*2], std::default_delete<uint8_t[]>());
            TileDataPtr tileData(new TileData(getOptions()));
            tileData->setEncoding(true);
            tileData->setIndex(tileIdx);
            tileData->setType(Options::GetEncodingCode(getOptions().getEncoding(), getOptions().isDeflate()));
            tileData->setPaletteData(ptrPalette);
            tileData->setIndexedData(ptrIndexed);
            tileData->setDeflatedData(ptrDeflated);
            tileData->setWidth(TILE_DIMENSION);
            tileData->setHeight(TILE_DIMENSION);
            // reading paletted tile
            if (fin.read(tileData->getPaletteData().get(), 1, PALETTE_SIZE) != PALETTE_SIZE) {
              return false;
            }
            if (fin.read(tileData->getIndexedData().get(), 1, tileSizeIndexed) != tileSizeIndexed) {
              return false;
            }
            pool->addTileData(tileData);
            tileIdx++;
          }

          // processing converted tiles
          while (pool->hasResult() && pool->peekResult() != nullptr &&
                 (unsigned)pool->peekResult()->getIndex() == nextTileIdx) {
            TileDataPtr retVal = pool->getResult();
            if (retVal == nullptr || retVal->isError()) {
              if (retVal != nullptr && !retVal->getErrorMsg().empty()) {
                std::printf("\n%s", retVal->getErrorMsg().c_str());
              }
              return false;
            }
            double ratio = 0.0;
            if (!writeEncodedTile(retVal, fout, ratio)) {
              return false;
            }
            if (getOptions().getVerbosity() == 1) {
              curProgress = showProgress(nextTileIdx, tileCount, curProgress, MAX_PROGRESS, '.');
            }
            ratioCount += ratio;
            nextTileIdx++;
          }
          if (tileIdx >= tileCount) {
            pool->waitForResult();
          }
        }
        if (getOptions().getVerbosity() == 1) std::printf("\n");

        if (nextTileIdx < tileCount) {
          std::printf("Missing tiles. Only %d of %d tiles converted.\n", nextTileIdx, tileCount);
          return false;
        }

        // displaying summary
        if (!getOptions().isSilent()) {
          std::printf("TIS file converted successfully. Total compression ratio: %.2f%%.\n",
                      ratioCount / (double)tileCount);
        }

        fout.setDeleteOnClose(false);
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
      unsigned compType, tileCount;

      // parsing TBC header
      if (!readTBC(fin, compType, tileCount)) return false;

      File fout(outFile.c_str(), "wb");
      fout.setDeleteOnClose(true);
      if (!fout.error()) {
        uint32_t v32;

        // writing TIS header
        if (fout.write(HEADER_TIS_SIGNATURE, 1, sizeof(HEADER_TIS_SIGNATURE)) != sizeof(HEADER_TIS_SIGNATURE)) return false;
        if (fout.write(HEADER_VERSION_V1, 1, sizeof(HEADER_VERSION_V1)) != sizeof(HEADER_VERSION_V1)) return false;
        v32 = get32u_le(&tileCount);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile count
        v32 = 0x1400; v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile size
        v32 = 0x18; v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing header size
        v32 = 0x40; v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile dimension

        if (getOptions().isVerbose()) {
          std::printf("Tile count: %d, encoding: %d - %s\n",
                      tileCount, compType, Options::GetEncodingName(compType).c_str());
        }
        if (getOptions().getVerbosity() == 1) std::printf("Converting");

        ThreadPoolPtr pool = createThreadPool(getOptions().getThreads(), MAX_POOL_TILES);
        unsigned tileIdx = 0, nextTileIdx = 0, curProgress = 0;
        while (tileIdx < tileCount || !pool->finished()) {
          // creating new tile data object
          if (tileIdx < tileCount) {
            uint32_t chunkSize;
            if (fin.read(&v32, 4, 1) != 1) return false;
            chunkSize = get32u_le(&v32);
            if (chunkSize == 0) {
              std::printf("\nInvalid block size found for tile #%d\n", tileIdx);
              return false;
            }
            BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
            BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
            BytePtr ptrDeflated(new uint8_t[chunkSize], std::default_delete<uint8_t[]>());
            if (fin.read(ptrDeflated.get(), 1, chunkSize) != chunkSize) return false;
            TileDataPtr tileData(new TileData(getOptions()));
            tileData->setEncoding(false);
            tileData->setIndex(tileIdx);
            tileData->setType(compType);
            tileData->setPaletteData(ptrPalette);
            tileData->setIndexedData(ptrIndexed);
            tileData->setDeflatedData(ptrDeflated);
            tileData->setSize(chunkSize);
            pool->addTileData(tileData);
            tileIdx++;
          }

          // writing converted tiles to disk
          while (pool->hasResult() && pool->peekResult() != nullptr &&
                 (unsigned)pool->peekResult()->getIndex() == nextTileIdx) {
            TileDataPtr retVal = pool->getResult();
            if (retVal == nullptr || retVal->isError()) {
              if (retVal != nullptr && !retVal->getErrorMsg().empty()) {
                std::printf("\n%s", retVal->getErrorMsg().c_str());
              }
              return false;
            }
            if (!writeDecodedTisTile(retVal, fout)) {
              return false;
            }
            if (getOptions().getVerbosity() == 1) {
              curProgress = showProgress(nextTileIdx, tileCount, curProgress, MAX_PROGRESS, '.');
            }
            nextTileIdx++;
          }
          if (tileIdx >= tileCount) {
            pool->waitForResult();
          }
        }
        if (getOptions().getVerbosity() == 1) std::printf("\n");

        if (nextTileIdx < tileCount) {
          std::printf("Missing tiles. Only %d of %d tiles converted.\n", nextTileIdx, tileCount);
          return false;
        }

        // displaying summary
        if (!getOptions().isSilent()) {
          std::printf("TBC file converted successfully.\n");
        }

        fout.setDeleteOnClose(false);
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
      unsigned mosWidth, mosHeight, mosCols, mosRows, palOfs;
      BytePtr mosData(nullptr);

      // loading MOS/MOSC input data
      if (!readMOS(fin, mosData, mosWidth, mosHeight, palOfs)) return false;
      mosCols = (mosWidth+63) >> 6;
      mosRows = (mosHeight+63) >> 6;

      File fout(outFile.c_str(), "wb");
      fout.setDeleteOnClose(true);
      if (!fout.error()) {
        uint32_t v32;
        uint32_t tileCount = mosCols * mosRows;
        uint32_t tileOfs = palOfs + tileCount*PALETTE_SIZE;   // start offset of dword array with relative offsets into tile data
        uint32_t dataOfs = tileOfs + tileCount*4;             // start offset of tile data

        // writing MBC header
        if (fout.write(HEADER_MBC_SIGNATURE, 1, sizeof(HEADER_MBC_SIGNATURE)) != sizeof(HEADER_MBC_SIGNATURE)) return false;
        if (fout.write(HEADER_VERSION_V1_0, 1, sizeof(HEADER_VERSION_V1_0)) != sizeof(HEADER_VERSION_V1_0)) return false;
        v32 = Options::GetEncodingCode(getOptions().getEncoding(), getOptions().isDeflate());
        v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing encoding type
        v32 = mosWidth; v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing MOS width
        v32 = mosHeight; v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing MOS height

        if (getOptions().isVerbose()) std::printf("Tile count: %d\n", tileCount);
        if (getOptions().getVerbosity() == 1) std::printf("Converting");

        // processing tiles
        ThreadPoolPtr pool = createThreadPool(getOptions().getThreads(), MAX_POOL_TILES);
        double ratioCount = 0.0;              // counts the compression ratios of all tiles
        unsigned tileIdx = 0, nextTileIdx = 0, curProgress = 0;
        while (tileIdx < tileCount || !pool->finished()) {
          // creating new tile data object
          if (tileIdx < tileCount) {
            int row = tileIdx / mosCols;
            int col = tileIdx % mosCols;
            int tileWidth = std::min(TILE_DIMENSION, mosWidth - col*TILE_DIMENSION);
            int tileHeight = std::min(TILE_DIMENSION, mosHeight - row*TILE_DIMENSION);
            BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
            BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
            BytePtr ptrDeflated(new uint8_t[MAX_TILE_SIZE_32*2], std::default_delete<uint8_t[]>());
            TileDataPtr tileData(new TileData(getOptions()));
            tileData->setEncoding(true);
            tileData->setIndex(tileIdx);
            tileData->setType(Options::GetEncodingCode(getOptions().getEncoding(), getOptions().isDeflate()));
            tileData->setPaletteData(ptrPalette);
            tileData->setIndexedData(ptrIndexed);
            tileData->setDeflatedData(ptrDeflated);
            tileData->setWidth(tileWidth);
            tileData->setHeight(tileHeight);
            // reading paletted tile
            std::memcpy(tileData->getPaletteData().get(), mosData.get()+palOfs, PALETTE_SIZE);
            palOfs += PALETTE_SIZE;
            // reading tile data
            v32 = get32u_le((uint32_t*)(mosData.get()+tileOfs));
            tileOfs += 4;
            std::memcpy(tileData->getIndexedData().get(), mosData.get()+dataOfs+v32, tileWidth*tileHeight);
            pool->addTileData(tileData);
            tileIdx++;
          }

          // writing converted tiles to disk
          while (pool->hasResult() && pool->peekResult() != nullptr &&
                 (unsigned)pool->peekResult()->getIndex() == nextTileIdx) {
            TileDataPtr retVal = pool->getResult();
            if (retVal == nullptr || retVal->isError()) {
              if (retVal != nullptr && !retVal->getErrorMsg().empty()) {
                std::printf("\n%s", retVal->getErrorMsg().c_str());
              }
              return false;
            }
            double ratio = 0.0;
            if (!writeEncodedTile(retVal, fout, ratio)) {
              return false;
            }
            if (getOptions().getVerbosity() == 1) {
              curProgress = showProgress(nextTileIdx, tileCount, curProgress, MAX_PROGRESS, '.');
            }
            ratioCount += ratio;
            nextTileIdx++;
          }
          if (tileIdx >= tileCount) {
            pool->waitForResult();
          }
        }
        if (getOptions().getVerbosity() == 1) std::printf("\n");

        if (nextTileIdx < tileCount) {
          std::printf("Missing tiles. Only %d of %d tiles converted.\n", nextTileIdx, tileCount);
          return false;
        }

        // displaying summary
        if (!getOptions().isSilent()) {
          std::printf("MOS file converted successfully. Total compression ratio: %.2f%%.\n",
                      ratioCount / (double)tileCount);
        }

        fout.setDeleteOnClose(false);
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
      unsigned compType, mosWidth, mosHeight;

      // parsing TBC header
      if (!readMBC(fin, compType, mosWidth, mosHeight)) return false;

      File fout(outFile.c_str(), "wb");
      fout.setDeleteOnClose(true);
      if (!fout.error()) {
        uint16_t v16;
        uint32_t v32;

        // creating a memory mapped copy of the output file
        uint32_t mosCols = (mosWidth + 63) >> 6;
        uint32_t mosRows = (mosHeight + 63) >> 6;
        uint32_t palOfs = 0x18;                                             // offset to palette data
        uint32_t tileOfs = palOfs + mosCols*mosRows*PALETTE_SIZE;           // offset to tile offset array
        uint32_t dataOfsBase = tileOfs + mosCols*mosRows*4, dataOfsRel = 0;     // abs. and rel. offsets to data blocks
        uint32_t mosSize = dataOfsBase + mosWidth*mosHeight;
        BytePtr mosData(new uint8_t[mosSize], std::default_delete<uint8_t[]>());

        // writing MOS header
        std::memcpy(mosData.get(), HEADER_MOS_SIGNATURE, 4);
        std::memcpy(mosData.get()+4, HEADER_VERSION_V1, 4);
        v16 = mosWidth;
        *(uint16_t*)(mosData.get()+8) = get16u_le(&v16);    // writing mos width
        v16 = mosHeight;
        *(uint16_t*)(mosData.get()+10) = get16u_le(&v16);   // writing mos height
        v16 = mosCols;
        *(uint16_t*)(mosData.get()+12) = get16u_le(&v16);   // writing mos columns
        v16 = mosRows;
        *(uint16_t*)(mosData.get()+14) = get16u_le(&v16);   // writing mos rows
        v32 = 0x40;
        *(uint32_t*)(mosData.get()+16) = get32u_le(&v32);   // writing tile dimension
        v32 = 0x18;
        *(uint32_t*)(mosData.get()+20) = get32u_le(&v32);   // writing offset to palettes

        if (getOptions().isVerbose()) {
          std::printf("Width: %d, height: %d, columns: %d, rows: %d, encoding: %d - %s\n",
                      mosWidth, mosHeight, mosCols, mosRows, compType, Options::GetEncodingName(compType).c_str());
        }
        if (getOptions().getVerbosity() == 1) std::printf("Converting");

        // processing tiles
        ThreadPoolPtr pool = createThreadPool(getOptions().getThreads(), MAX_POOL_TILES);
        uint32_t tileCount = mosCols * mosRows;
        uint32_t tileIdx = 0, nextTileIdx = 0, curProgress = 0;
        while (tileIdx < tileCount || !pool->finished()) {
          // creating new tile data object
          if (tileIdx < tileCount) {
            unsigned chunkSize;
            if (fin.read(&v32, 4, 1) != 1) return false;
            chunkSize = get32u_le(&v32);
            if (chunkSize == 0) {
              std::printf("\nInvalid block size found for tile #%d\n", tileIdx);
              return false;
            }
            BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
            BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
            BytePtr ptrDeflated(new uint8_t[chunkSize], std::default_delete<uint8_t[]>());
            if (fin.read(ptrDeflated.get(), 1, chunkSize) != chunkSize) return false;
            TileDataPtr tileData(new TileData(getOptions()));
            tileData->setEncoding(false);
            tileData->setIndex(tileIdx);
            tileData->setType(compType);
            tileData->setPaletteData(ptrPalette);
            tileData->setIndexedData(ptrIndexed);
            tileData->setDeflatedData(ptrDeflated);
            tileData->setSize(chunkSize);
            pool->addTileData(tileData);
            tileIdx++;
          }

          // writing converted tiles to disk
          while (pool->hasResult() && pool->peekResult() != nullptr &&
                 (unsigned)pool->peekResult()->getIndex() == nextTileIdx) {
            TileDataPtr retVal = pool->getResult();
            if (retVal == nullptr || retVal->isError()) {
              if (retVal != nullptr && !retVal->getErrorMsg().empty()) {
                std::printf("\n%s", retVal->getErrorMsg().c_str());
              }
              return false;
            }
            if (!writeDecodedMosTile(retVal, mosData, palOfs, tileOfs, dataOfsRel, dataOfsBase)) {
              return false;
            }
            if (getOptions().getVerbosity() == 1) {
              curProgress = showProgress(nextTileIdx, tileCount, curProgress, MAX_PROGRESS, '.');
            }
            nextTileIdx++;
          }
          if (tileIdx >= tileCount) {
            pool->waitForResult();
          }
        }
        if (getOptions().getVerbosity() == 1) std::printf("\n");

        if (nextTileIdx < tileCount) {
          std::printf("Missing tiles. Only %d of %d tiles converted.\n", nextTileIdx, tileCount);
          return false;
        }

        // writing MOS/MOSC to disk
        if (!writeMos(fout, mosData, mosSize)) return false;

        // displaying summary
        if (!getOptions().isSilent()) {
          std::printf("MBC file converted successfully.\n");
        }

        fout.setDeleteOnClose(false);
        return true;
      }
    } else {
      std::printf("Error opening file \"%s\"\n", inFile.c_str());
    }
  }
  return false;
}


bool Graphics::tizToTIS(const std::string &inFile, const std::string &outFile) noexcept
{
  if (!inFile.empty() && !outFile.empty() && inFile != outFile) {
    File fin(inFile.c_str(), "rb");
    if (!fin.error()) {
      char tsig[4];
      unsigned compType, tileCount;

      // parsing TIZ header
      if (!readTIZ(fin, compType, tileCount)) return false;

      File fout(outFile.c_str(), "wb");
      fout.setDeleteOnClose(true);
      if (!fout.error()) {
        uint32_t v32;

        // writing TIS header
        if (fout.write(HEADER_TIS_SIGNATURE, 1, sizeof(HEADER_TIS_SIGNATURE)) != sizeof(HEADER_TIS_SIGNATURE)) return false;
        if (fout.write(HEADER_VERSION_V1, 1, sizeof(HEADER_VERSION_V1)) != sizeof(HEADER_VERSION_V1)) return false;
        v32 = get32u_le(&tileCount);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile count
        v32 = 0x1400; v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile size
        v32 = 0x18; v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing header size
        v32 = 0x40; v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing tile dimension

        if (getOptions().isVerbose()) {
          std::printf("Tile count: %d, encoding: %d - %s\n",
                      tileCount, compType, Options::GetEncodingName(compType).c_str());
        }
        if (getOptions().getVerbosity() == 1) std::printf("Converting");

        // processing tiles
        ThreadPoolPtr pool = createThreadPool(getOptions().getThreads(), MAX_POOL_TILES);
        unsigned tileIdx = 0, nextTileIdx = 0, curProgress = 0;
        while (tileIdx < tileCount || !pool->finished()) {
          if (tileIdx < tileCount) {
            // creating new tile data object
            uint32_t chunkSize;
            BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
            BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
            BytePtr ptrDeflated;

            if (fin.read(tsig, 1, 4) != 4) return false;
            if (std::strncmp(tsig, HEADER_TIL0_SIGNATURE, 4) == 0 ||
                std::strncmp(tsig, HEADER_TIL1_SIGNATURE, 4) == 0 ||
                std::strncmp(tsig, HEADER_TIL2_SIGNATURE, 4) == 0) {
              uint16_t tileSize;
              if (fin.read(&tileSize, 2, 1) != 1) return false;
              chunkSize = get16u_be(&tileSize);
              ptrDeflated.reset(new uint8_t[chunkSize+6], std::default_delete<uint8_t[]>());
              std::memcpy(ptrDeflated.get(), tsig, 4);
              std::memcpy(ptrDeflated.get()+4, &tileSize, 2);
              if (fin.read(ptrDeflated.get()+6, 1, chunkSize) != chunkSize) return false;
              chunkSize += 6;
            } else {
              std::printf("\nInvalid header found in tile #%d\n", tileIdx);
              return false;
            }
            TileDataPtr tileData(new TileData(getOptions()));
            tileData->setEncoding(false);
            tileData->setIndex(tileIdx);
            tileData->setType(compType);
            tileData->setPaletteData(ptrPalette);
            tileData->setIndexedData(ptrIndexed);
            tileData->setDeflatedData(ptrDeflated);
            tileData->setSize(chunkSize);
            pool->addTileData(tileData);
            tileIdx++;
          }

          // writing converted tiles to disk
          while (pool->hasResult() && pool->peekResult() != nullptr &&
                 (unsigned)pool->peekResult()->getIndex() == nextTileIdx) {
            TileDataPtr retVal = pool->getResult();
            if (retVal == nullptr || retVal->isError()) {
              if (retVal != nullptr && !retVal->getErrorMsg().empty()) {
                std::printf("\n%s", retVal->getErrorMsg().c_str());
              }
              return false;
            }
            if (!writeDecodedTisTile(retVal, fout)) {
              return false;
            }
            if (getOptions().getVerbosity() == 1) {
              curProgress = showProgress(nextTileIdx, tileCount, curProgress, MAX_PROGRESS, '.');
            }
            nextTileIdx++;
          }
          if (tileIdx >= tileCount) {
            pool->waitForResult();
          }
        }
        if (getOptions().getVerbosity() == 1) std::printf("\n");

        if (nextTileIdx < tileCount) {
          std::printf("Missing tiles. Only %d of %d tiles converted.\n", nextTileIdx, tileCount);
          return false;
        }

        // displaying summary
        if (!getOptions().isSilent()) {
          std::printf("TIZ file converted successfully.\n");
        }

        fout.setDeleteOnClose(false);
        return true;
      }
    } else {
      std::printf("Error opening file \"%s\"\n", inFile.c_str());
    }
  }
  return false;
}


bool Graphics::mozToMOS(const std::string &inFile, const std::string &outFile) noexcept
{
  if (!inFile.empty() && !outFile.empty() && inFile != outFile) {
    File fin(inFile.c_str(), "rb");
    if (!fin.error()) {
      char tsig[4];
      unsigned compType, mosWidth, mosHeight;

      // parsing TIZ header
      if (!readMOZ(fin, compType, mosWidth, mosHeight)) return false;

      File fout(outFile.c_str(), "wb");
      fout.setDeleteOnClose(true);
      if (!fout.error()) {
        uint16_t v16;
        uint32_t v32;

        // creating a memory mapped copy of the output file
        uint32_t mosCols = (mosWidth + 63) >> 6;
        uint32_t mosRows = (mosHeight + 63) >> 6;
        uint32_t palOfs = 0x18;                                             // offset to palette data
        uint32_t tileOfs = palOfs + mosCols*mosRows*PALETTE_SIZE;           // offset to tile offset array
        uint32_t dataOfsBase = tileOfs + mosCols*mosRows*4, dataOfsRel = 0;     // abs. and rel. offsets to data blocks
        uint32_t mosSize = dataOfsBase + mosWidth*mosHeight;
        BytePtr mosData(new uint8_t[mosSize], std::default_delete<uint8_t[]>());

        // writing MOS header
        std::memcpy(mosData.get(), HEADER_MOS_SIGNATURE, 4);
        std::memcpy(mosData.get()+4, HEADER_VERSION_V1, 4);
        v16 = mosWidth;
        *(uint16_t*)(mosData.get()+8) = get16u_le(&v16);    // writing mos width
        v16 = mosHeight;
        *(uint16_t*)(mosData.get()+10) = get16u_le(&v16);   // writing mos height
        v16 = mosCols;
        *(uint16_t*)(mosData.get()+12) = get16u_le(&v16);   // writing mos columns
        v16 = mosRows;
        *(uint16_t*)(mosData.get()+14) = get16u_le(&v16);   // writing mos rows
        v32 = 0x40;
        *(uint32_t*)(mosData.get()+16) = get32u_le(&v32);   // writing tile dimension
        v32 = 0x18;
        *(uint32_t*)(mosData.get()+20) = get32u_le(&v32);   // writing offset to palettes

        if (getOptions().isVerbose()) {
          std::printf("Width: %d, height: %d, columns: %d, rows: %d, encoding: %d - %s\n",
                      mosWidth, mosHeight, mosCols, mosRows, compType, Options::GetEncodingName(compType).c_str());
        }
        if (getOptions().getVerbosity() == 1) std::printf("Converting");

        // processing tiles
        ThreadPoolPtr pool = createThreadPool(getOptions().getThreads(), MAX_POOL_TILES);
        uint32_t tileCount = mosCols * mosRows;
        uint32_t tileIdx = 0, nextTileIdx = 0, curProgress = 0;
        while (tileIdx < tileCount || !pool->finished()) {
          // creating new tile data object
          if (tileIdx < tileCount) {
            uint32_t chunkSize;
            BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
            BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
            BytePtr ptrDeflated;

            if (fin.read(tsig, 1, 4) != 4) return false;
            if (std::strncmp(tsig, HEADER_TIL2_SIGNATURE, 4) == 0) {
              // jpeg compressed tile
              uint16_t tileSize;
              if (fin.read(&tileSize, 2, 1) != 1) return false;
              chunkSize = get16u_be(&tileSize);
              ptrDeflated.reset(new uint8_t[chunkSize+6], std::default_delete<uint8_t[]>());
              std::memcpy(ptrDeflated.get(), tsig, 4);
              std::memcpy(ptrDeflated.get()+4, &tileSize, 2);
              if (fin.read(ptrDeflated.get()+6, 1, chunkSize) != chunkSize) return false;
              chunkSize += 6;
            } else {
              std::printf("\nInvalid header found in tile #%d\n", tileIdx);
              return false;
            }
            TileDataPtr tileData(new TileData(getOptions()));
            tileData->setEncoding(false);
            tileData->setIndex(tileIdx);
            tileData->setType(compType);
            tileData->setPaletteData(ptrPalette);
            tileData->setIndexedData(ptrIndexed);
            tileData->setDeflatedData(ptrDeflated);
            tileData->setSize(chunkSize);
            pool->addTileData(tileData);
            tileIdx++;
          }

          // writing converted tiles to disk
          while (pool->hasResult() && pool->peekResult() != nullptr &&
                 (unsigned)pool->peekResult()->getIndex() == nextTileIdx) {
            TileDataPtr retVal = pool->getResult();
            if (retVal == nullptr || retVal->isError()) {
              if (retVal != nullptr && !retVal->getErrorMsg().empty()) {
                std::printf("\n%s", retVal->getErrorMsg().c_str());
              }
              return false;
            }
            if (!writeDecodedMosTile(retVal, mosData, palOfs, tileOfs, dataOfsRel, dataOfsBase)) {
              return false;
            }
            if (getOptions().getVerbosity() == 1) {
              curProgress = showProgress(nextTileIdx, tileCount, curProgress, MAX_PROGRESS, '.');
            }
            nextTileIdx++;
          }
          if (tileIdx >= tileCount) {
            pool->waitForResult();
          }
        }
        if (getOptions().getVerbosity() == 1) std::printf("\n");

        if (nextTileIdx < tileCount) {
          std::printf("Missing tiles. Only %d of %d tiles converted.\n", nextTileIdx, tileCount);
          return false;
        }

        // writing MOS/MOSC to disk
        if (!writeMos(fout, mosData, mosSize)) return false;

        // displaying summary
        if (!getOptions().isSilent()) {
          std::printf("MOZ file converted successfully.\n");
        }

        fout.setDeleteOnClose(false);
        return true;
      }
    } else {
      std::printf("Error opening file \"%s\"\n", inFile.c_str());
    }
  }
  return false;
}


bool Graphics::readTIS(File &fin, unsigned &numTiles) noexcept
{
  char id[4];
  bool isHeaderless = false;
  uint32_t v32;

  if (fin.read(id, 1, 4) != 4) return false;
  if (std::strncmp(id, HEADER_TIS_SIGNATURE, 4) != 0) {
    if (getOptions().assumeTis()) {
      isHeaderless = true;
    } else {
      std::printf("Invalid TIS signature\n");
      return false;
    }
  }

  if (!isHeaderless) {
    if (fin.read(id, 1, 4) != 4) return false;
    if (std::strncmp(id, HEADER_VERSION_V2, 4) == 0) {
      if (!getOptions().isSilent()) {
        std::printf("Warning: Incorrect TIS version 2 found. Converting anyway.\n");
      }
    } else if (std::strncmp(id, HEADER_VERSION_V1, 4) != 0) {
      std::printf("Invalid TIS version\n");
      return false;
    }

    if (fin.read(&v32, 4, 1) != 1) return false;
    numTiles = get32u_le(&v32);
    if (numTiles == 0) {
      std::printf("No tiles found\n");
      return false;
    }

    if (fin.read(&v32, 4, 1) != 1) return false;
    v32 = get32u_le(&v32);
    if (v32 != 0x1400) {
      if (v32 == 0x000c) {
        std::printf("PVRZ-based TIS files are not supported\n");
      } else {
        std::printf("Invalid tile size\n");
      }
      return false;
    }

    if (fin.read(&v32, 4, 1) != 1) return false;
    v32 = get32u_le(&v32);
    if (v32 < 0x18) {
      std::printf("Invalid header size\n");
      return false;
    }

    if (fin.read(&v32, 4, 1) != 1) return false;
    v32 = get32u_le(&v32);
    if (v32 != 0x40) {
      std::printf("Invalid tile dimensions\n");
      return false;
    }
  } else {
    long size = fin.getsize();
    fin.seek(0L, SEEK_SET);
    if (size < 0L) {
      std::printf("Error reading input file\n");
      return false;
    } else if ((size % 5120) != 0) {
      std::printf("Headerless TIS has wrong file size\n");
      return false;
    } else {
      if (!getOptions().isSilent()) std::printf("Warning: Headerless TIS file detected\n");
      numTiles = (unsigned)size / 0x1400;
    }
  }

  return true;
}


bool Graphics::readMOS(File &fin, BytePtr &mos, unsigned &width, unsigned &height,
                       unsigned &palOfs) noexcept
{
  char id[4];
  uint32_t v32, mosSize;

  // loading MOS/MOSC input file
  if (fin.read(id, 1, 4) != 4) return false;;
  if (std::strncmp(id, HEADER_MOSC_SIGNATURE, 4) == 0) {    // decompressing MOSC
    uint32_t moscSize;
    Compression compression;

    // getting MOSC file size
    moscSize = fin.getsize();
    if (moscSize <= 12) {
      std::printf("Invalid MOSC size\n");
      return false;
    }
    moscSize -= 12;    // removing header size

    if (fin.read(&id, 1, 4) != 4) return false;
    if (std::strncmp(id, HEADER_VERSION_V1, 4) != 0) {
      std::printf("Invalid MOSC version\n");
      return false;
    }

    if (fin.read(&v32, 4, 1) != 1) return false;
    mosSize = get32u_le(&v32);
    if (mosSize < 24) {
      std::printf("MOS size too small\n");
      return false;
    }
    BytePtr moscData(new uint8_t[moscSize], std::default_delete<uint8_t[]>());
    if (fin.read(moscData.get(), 1, moscSize) < moscSize) {
      std::printf("Incomplete or corrupted MOSC file\n");
      return false;
    }

    mos.reset(new uint8_t[mosSize], std::default_delete<uint8_t[]>());
    unsigned size = compression.inflate(moscData.get(), moscSize, mos.get(), mosSize);
    if (size != mosSize) {
      std::printf("Error while decompressing MOSC input file\n");
      return false;
    }
  } else if (std::strncmp(id, HEADER_MOS_SIGNATURE, 4) == 0) {   // loading MOS data
    mosSize = fin.getsize();
    if (mosSize < 24) {
      std::printf("MOS size too small\n");
      return false;
    }
    fin.seek(0, SEEK_SET);
    mos.reset(new uint8_t[mosSize], std::default_delete<uint8_t[]>());
    if (fin.read(mos.get(), 1, mosSize) != mosSize) return false;
  } else {
    std::printf("Invalid MOS signature\n");
    return false;
  }

  // parsing MOS header
  uint32_t inOfs = 0;
  if (std::memcmp(mos.get()+inOfs, HEADER_MOS_SIGNATURE, 4) != 0) {
    std::printf("Invalid MOS signature\n");
    return false;
  }
  inOfs += 4;

  if (std::memcmp(mos.get()+inOfs, HEADER_VERSION_V1, 4) != 0) {
    std::printf("Unsupported MOS version\n");
    return false;
  }
  inOfs += 4;

  width = get16u_le((uint16_t*)(mos.get()+inOfs));
  if (width == 0) {
    std::printf("Invalid MOS width\n");
    return false;
  }
  inOfs += 2;

  height = get16u_le((uint16_t*)(mos.get()+inOfs));
  if (height == 0) {
    std::printf("Invalid MOS height\n");
    return false;
  }
  inOfs += 2;

  if (get16u_le((uint16_t*)(mos.get()+inOfs)) == 0) {
    std::printf("Invalid number of tiles\n");
    return false;
  }
  inOfs += 2;

  if (get16u_le((uint16_t*)(mos.get()+inOfs)) == 0) {
    std::printf("Invalid number of tiles\n");
    return false;
  }
  inOfs += 2;

  if (get32u_le((uint32_t*)(mos.get()+inOfs)) != 0x40) {
    std::printf("Invalid tile dimensions\n");
    return false;
  }
  inOfs += 4;

  palOfs = get32u_le((uint32_t*)(mos.get()+inOfs));
  if (palOfs < 24) {
    std::printf("MOS header too small\n");
    return false;
  }
  inOfs = palOfs;

  {
    unsigned cols = (width+63) >> 6;
    unsigned rows = (height+63) >> 6;
    // comparing calculated size with actual input file length
    uint32_t size = palOfs + cols*rows*PALETTE_SIZE + cols*rows*4 + width*height;
    if (mosSize < size) {
      std::printf("Incomplete or corrupted MOS file\n");
      return false;
    }
  }

  return true;
}


bool Graphics::readTBC(File &fin, unsigned &type, unsigned &numTiles) noexcept
{
  char id[4];
  uint32_t v32;

  // parsing TBC header
  if (fin.read(id, 1, 4) != 4) return false;;
  if (std::strncmp(id, HEADER_TBC_SIGNATURE, 4) != 0) {
    std::printf("Invalid TBC signature\n");
    return false;
  }

  if (fin.read(id, 1, 4) != 4) return false;
  if (std::strncmp(id, HEADER_VERSION_V1_0, 4) != 0) {
    std::printf("Unsupported TBC version\n");
    return false;
  }

  if (fin.read(&v32, 4, 1) != 1) return false;
  type = get32u_le(&v32);

  if (fin.read(&v32, 4, 1) != 1) return false;
  numTiles = get32u_le(&v32);
  if (numTiles == 0) {
    std::printf("No tiles found\n");
    return false;
  }

  return true;
}


bool Graphics::readMBC(File &fin, unsigned &type, unsigned &width, unsigned &height) noexcept
{
  char id[4];
  uint32_t v32;

  // parsing TBC header
  if (fin.read(id, 1, 4) != 4) return false;;
  if (std::strncmp(id, HEADER_MBC_SIGNATURE, 4) != 0) {
    std::printf("Invalid MBC signature\n");
    return false;
  }

  if (fin.read(id, 1, 4) != 4) return false;
  if (std::strncmp(id, HEADER_VERSION_V1_0, 4) != 0) {
    std::printf("Invalid MBC version\n");
    return false;
  }

  if (fin.read(&v32, 4, 1) != 1) return false;
  type = get32u_le(&v32);

  if (fin.read(&v32, 4, 1) != 1) return false;
  width = get32u_le(&v32);
  if (width == 0) {
    std::printf("Invalid MBC width\n");
    return false;
  }

  if (fin.read(&v32, 4, 1) != 1) return false;
  height = get32u_le(&v32);
  if (height == 0) {
    std::printf("Invalid MBC height\n");
    return false;
  }

  return true;
}


bool Graphics::readTIZ(File &fin, unsigned &type, unsigned &numTiles) noexcept
{
  char id[4];
  uint16_t v16;

  type = Options::GetEncodingCode(Encoding::Z, false);

  // parsing TIZ header
  if (fin.read(id, 1, 4) != 4) return false;;
  if (std::strncmp(id, HEADER_TIZ_SIGNATURE, 4) != 0) {
    std::printf("Invalid TIZ signature\n");
    return false;
  }

  if (fin.read(&v16, 2, 1) != 1) return false;
  numTiles = get16u_be(&v16);
  if (numTiles == 0) {
    std::printf("No tiles found\n");
    return false;
  }

  // skipping 2 bytes
  if (fin.read(&v16, 2, 1) != 1) return false;

  return true;
}


bool Graphics::readMOZ(File &fin, unsigned &type, unsigned &width, unsigned &height) noexcept
{
  char id[4];
  uint16_t v16;

  type = Options::GetEncodingCode(Encoding::Z, false);

  // parsing TIZ header
  if (fin.read(id, 1, 4) != 4) return false;;
  if (std::strncmp(id, HEADER_MOZ_SIGNATURE, 4) != 0) {
    std::printf("Invalid MOZ signature\n");
    return false;
  }

  if (fin.read(&v16, 2, 1) != 1) return false;
  width = get16u_be(&v16);
  if (width == 0) {
    std::printf("Invalid MOZ width\n");
    return false;
  }

  if (fin.read(&v16, 2, 1) != 1) return false;
  height = get16u_be(&v16);
  if (height == 0) {
    std::printf("Invalid MOZ height\n");
    return false;
  }

  return true;
}


bool Graphics::writeMos(File &fout, BytePtr &mos, unsigned size) noexcept
{
  if (mos != nullptr && size > 0) {
    if (getOptions().isMosc()) {
      // compressing mos -> mosc
      uint32_t moscSize = size*2;
      uint32_t v32;
      Compression compression;
      BytePtr moscData(new uint8_t[moscSize], std::default_delete<uint8_t[]>());

      // writing MOSC header
      std::memcpy(moscData.get(), HEADER_MOSC_SIGNATURE, 4);
      std::memcpy(moscData.get()+4, HEADER_VERSION_V1, 4);
      v32 = size;
      *(uint32_t*)(moscData.get()+8) = get32u_le(&v32);

      // compressing data
      moscSize = compression.deflate(mos.get(), size, moscData.get()+12, moscSize-12);
      moscSize += 12;

      // writing cmos data to file
      if (fout.write(moscData.get(), 1, moscSize) == moscSize) return true;
    } else {
      // writing mos data to file
      if (fout.write(mos.get(), 1, size) == size) return true;
    }
  }
  return false;
}


bool Graphics::writeEncodedTile(TileDataPtr tileData, File &file, double &ratio) noexcept
{
  if (tileData != nullptr) {
    if (tileData->getSize() > 0 && !tileData->isError()) {
      uint32_t v32 = tileData->getSize(); v32 = get32u_le(&v32);  // compressed tile size in ready-to-write format
      if (file.write(&v32, 4, 1) != 1) {
        std::printf("Error while writing tile data\n");
        return false;
      }
      if (file.write(tileData->getDeflatedData().get(), 1, tileData->getSize()) != (unsigned)tileData->getSize()) {
        std::printf("Error while writing tile data\n");
        return false;
      }

      // displaying statistical information
      int tileSizeIndexed = tileData->getWidth() * tileData->getHeight();
      ratio = ((double)(tileData->getSize()+HEADER_TILE_COMPRESSED_SIZE)*100.0) / (double)(tileSizeIndexed+PALETTE_SIZE);
      if (getOptions().isVerbose()) {
        std::printf("Tile #%d finished. Original size = %d bytes. Compressed size = %d bytes. Compression ratio: %.2f%%.\n",
                    tileData->getIndex(), tileSizeIndexed+PALETTE_SIZE, tileData->getSize()+HEADER_TILE_COMPRESSED_SIZE, ratio);
      }
      return true;
    } else {
      std::printf("%s", tileData->getErrorMsg().c_str());
    }
  }
  return false;
}


bool Graphics::writeDecodedTisTile(TileDataPtr tileData, File &file) noexcept
{
  if (tileData != nullptr) {
    if (tileData->getSize() > 0 && !tileData->isError()) {
      if (file.write(tileData->getPaletteData().get(), 1, PALETTE_SIZE) != PALETTE_SIZE) {
        std::printf("Error while writing tile data\n");
        return false;
      }
      if (file.write(tileData->getIndexedData().get(), 1, MAX_TILE_SIZE_8) != MAX_TILE_SIZE_8) {
        std::printf("Error while writing tile data\n");
        return false;
      }

      if (getOptions().isVerbose()) {
        std::printf("Tile #%d decoded successfully\n", tileData->getIndex());
      }
      return true;
    } else {
      std::printf("%s", tileData->getErrorMsg().c_str());
    }
  }
  return false;
}


bool Graphics::writeDecodedMosTile(TileDataPtr tileData, BytePtr mosData, uint32_t &palOfs,
                                   uint32_t &tileOfs, uint32_t &dataOfsRel, uint32_t dataOfsBase) noexcept
{
  if (tileData != nullptr && mosData != nullptr) {
    if (tileData->getSize() > 0 && !tileData->isError()) {
      uint32_t tileSizeIndexed = tileData->getWidth() * tileData->getHeight();

      // writing palette data
      std::memcpy(mosData.get()+palOfs, tileData->getPaletteData().get(), PALETTE_SIZE);
      palOfs += PALETTE_SIZE;

      // writing tile offsets
      uint32_t v32 = get32u_le(&dataOfsRel);
      std::memcpy(mosData.get()+tileOfs, &v32, 4);
      tileOfs += 4;

      // writing tile data
      std::memcpy(mosData.get()+dataOfsBase+dataOfsRel, tileData->getIndexedData().get(), tileSizeIndexed);
      dataOfsRel += tileSizeIndexed;

      if (getOptions().isVerbose()) {
        std::printf("Tile #%d decoded successfully\n", tileData->getIndex());
      }
      return true;
    } else {
      std::printf("%s", tileData->getErrorMsg().c_str());
    }
  }
  return false;
}


unsigned Graphics::showProgress(unsigned curTile, unsigned maxTiles,
                                unsigned curProgress, unsigned maxProgress,
                                char symbol) const noexcept
{
  curTile++;
  if (curTile > maxTiles) curTile = maxTiles;
  if (curProgress > maxProgress) curProgress = maxProgress;
  unsigned v = curTile*maxProgress / maxTiles;
  while (curProgress < v) {
    std::printf("%c", symbol);
#ifndef WIN32
    std::fflush(stdout);
#endif
    curProgress++;
  }
  return v;
}

}   // namespace tc
