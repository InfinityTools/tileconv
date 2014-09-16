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

const unsigned Graphics::MAX_PROGRESS                = 69;

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
      bool isHeaderless = false;

      // parsing TIS header
      if (fin.read(sig, 1, 4) != 4) return false;
      if (std::strncmp(sig, HEADER_TIS_SIGNATURE, 4) != 0) {
        if (getOptions().assumeTis()) {
          fin.seek(0L, SEEK_SET);
          isHeaderless = true;
        } else {
          std::printf("Invalid TIS signature\n");
          return false;
        }
      }

      if (!isHeaderless) {
        if (fin.read(ver, 1, 4) != 4) return false;
        if (std::strncmp(ver, HEADER_VERSION_V2, 4) == 0) {
          if (!getOptions().isSilent()) {
            std::printf("Warning: Incorrect TIS version 2 found. Converting anyway.\n");
          }
        } else if (std::strncmp(ver, HEADER_VERSION_V1, 4) != 0) {
          std::printf("Invalid TIS version\n");
          return false;
        }

        if (fin.read(&tileCount, 4, 1) != 1) return false;
        tileCount = get32u_le(&tileCount);
        if (tileCount == 0) {
          std::printf("No tiles found\n");
          return false;
        }

        if (fin.read(&tileSize, 4, 1) != 1) return false;
        tileSize = get32u_le(&tileSize);
        if (tileSize != 0x1400) {
          if (tileSize == 0x000c) {
            std::printf("PVRZ-based TIS files are not supported\n");
          } else {
            std::printf("Invalid tile size\n");
          }
          return false;
        }

        if (fin.read(&headerSize, 4, 1) != 1) return false;
        headerSize = get32u_le(&headerSize);
        if (headerSize < 0x18) {
          std::printf("Invalid header size\n");
          return false;
        }

        if (fin.read(&tileDim, 4, 1) != 1) return false;
        tileDim = get32u_le(&tileDim);
        if (tileDim != 0x40) {
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
          tileSize = 0x1400;
          tileDim = 0x40;
          tileCount = (uint32_t)(size / tileSize);
        }
      }

      File fout(outFile.c_str(), "wb");
      if (!fout.error()) {
        uint32_t v32;
        uint32_t tileSizeIndexed = tileDim*tileDim;   // indexed size of the tile

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
        ThreadPoolPtr pool = createThreadPool(*this, getOptions().getThreads(), 64);
        unsigned nextTileIdx = 0, curProgress = 0;
        double ratioCount = 0.0;    // counts the compression ratios of all tiles
        for (unsigned tileIdx = 0; tileIdx < tileCount; tileIdx++) {
          if (getOptions().isVerbose()) std::printf("Converting tile #%d\n", tileIdx);

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

          // creating new tile data object
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
          tileData->setWidth(tileDim);
          tileData->setHeight(tileDim);
          // reading paletted tile
          if (fin.read(tileData->getPaletteData().get(), 1, PALETTE_SIZE) != PALETTE_SIZE) {
            return false;
          }
          if (fin.read(tileData->getIndexedData().get(), 1, tileSizeIndexed) != tileSizeIndexed) {
            return false;
          }
          pool->addTileData(tileData);
        }

        // retrieving the remaining tile data blocks in queue
        while (!pool->finished()) {
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
          pool->waitForResult();
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
      if (std::strncmp(ver, HEADER_VERSION_V1_0, 4) != 0) {
        std::printf("Unsupported TBC version\n");
        return false;
      }

      if (fin.read(&compType, 4, 1) != 1) return false;
      compType = get32u_le(&compType);

      if (fin.read(&tileCount, 4, 1) != 1) return false;
      tileCount = get32u_le(&tileCount);
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

        ThreadPoolPtr pool = createThreadPool(*this, getOptions().getThreads(), 64);
        uint32_t nextTileIdx = 0, curProgress = 0;
        for (uint32_t tileIdx = 0; tileIdx < tileCount; tileIdx++) {
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

          // creating new tile data object
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
        }

        // retrieving the remaining tile data blocks in queue
        while (!pool->finished()) {
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
          pool->waitForResult();
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
        moscSize = fin.getsize();
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

        if (fin.read(&mosSize, 4, 1) != 1) return false;
        mosSize = get32u_le(&mosSize);
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
        mosSize = fin.getsize();
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
      mosWidth = get16u_le(&mosWidth);
      if (mosWidth == 0) {
        std::printf("Invalid MOS width\n");
        return false;
      }
      inOfs += 2;

      mosHeight = get16u_le((uint16_t*)(mosData.get()+inOfs));
      if (mosHeight == 0) {
        std::printf("Invalid MOS height\n");
        return false;
      }
      inOfs += 2;

      mosCols = get16u_le((uint16_t*)(mosData.get()+inOfs));
      if (mosCols == 0) {
        std::printf("Invalid number of tiles\n");
        return false;
      }
      inOfs += 2;

      mosRows = get16u_le((uint16_t*)(mosData.get()+inOfs));
      if (mosRows == 0) {
        std::printf("Invalid number of tiles\n");
        return false;
      }
      inOfs += 2;

      tileDim = get32u_le((uint32_t*)(mosData.get()+inOfs));
      if (tileDim != 0x40) {
        std::printf("Invalid tile dimensions\n");
        return false;
      }
      inOfs += 4;

      palOfs = get32u_le((uint32_t*)(mosData.get()+inOfs));
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
        v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing encoding type
        v32 = mosWidth; v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing MOS width
        v32 = mosHeight; v32 = get32u_le(&v32);
        if (fout.write(&v32, 4, 1) != 1) return false;    // writing MOS height
        if (getOptions().isVerbose()) std::printf("Tile count: %d\n", tileCount);
        if (getOptions().getVerbosity() == 1) std::printf("Converting");

        // processing tiles
        ThreadPoolPtr pool = createThreadPool(*this, getOptions().getThreads(), 64);
        unsigned nextTileIdx = 0, curProgress = 0;
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

            // creating new tile data object
            BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
            BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
            BytePtr ptrDeflated(new uint8_t[MAX_TILE_SIZE_32*2], std::default_delete<uint8_t[]>());
            TileDataPtr tileData(new TileData(getOptions()));
            tileData->setEncoding(true);
            tileData->setIndex(curIndex);
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
            std::memcpy(&v32, mosData.get()+tileOfs, 4);
            v32 = get32u_le(&v32);
            tileOfs += 4;
            std::memcpy(tileData->getIndexedData().get(), mosData.get()+dataOfs+v32, tileSizeIndexed);
            pool->addTileData(tileData);
          }
        }

        // retrieving the remaining tile data blocks in queue
        while (!pool->finished()) {
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
          pool->waitForResult();
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
      compType = get32u_le(&compType);

      if (fin.read(&mosWidth, 4, 1) != 1) return false;
      mosWidth = get32u_le(&mosWidth);
      if (mosWidth == 0) {
        std::printf("Invalid MBC width\n");
        return false;
      }

      if (fin.read(&mosHeight, 4, 1) != 1) return false;
      mosHeight = get32u_le(&mosHeight);
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
        std::memcpy(mosData.get()+headerOfs, HEADER_MOS_SIGNATURE, 4);
        headerOfs += 4;
        std::memcpy(mosData.get()+headerOfs, HEADER_VERSION_V1, 4);
        headerOfs += 4;
        v16 = mosWidth; v16 = get16u_le(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos width
        headerOfs += 2;
        v16 = mosHeight; v16 = get16u_le(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos height
        headerOfs += 2;
        v16 = mosCols; v16 = get16u_le(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos columns
        headerOfs += 2;
        v16 = mosRows; v16 = get16u_le(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos rows
        headerOfs += 2;
        v32 = 0x40; v32 = get32u_le(&v32);
        std::memcpy(mosData.get()+headerOfs, &v32, 4);     // writing tile dimension
        headerOfs += 4;
        v32 = 0x18; v32 = get32u_le(&v32);
        std::memcpy(mosData.get()+headerOfs, &v32, 4);     // writing offset to palettes
        headerOfs += 4;

        if (getOptions().isVerbose()) {
          std::printf("Width: %d, height: %d, columns: %d, rows: %d, encoding: %d - %s\n",
                      mosWidth, mosHeight, mosCols, mosRows, compType, Options::GetEncodingName(compType).c_str());
        }
        if (getOptions().getVerbosity() == 1) std::printf("Converting");

        ThreadPoolPtr pool = createThreadPool(*this, getOptions().getThreads(), 64);
        uint32_t tileCount = mosCols * mosRows;
        uint32_t nextTileIdx = 0, curProgress = 0;
        int curIndex = 0;                       // the current tile index
        for (uint32_t row = 0; row < mosRows; row++) {
          for (uint32_t col = 0; col < mosCols; col++, curIndex++) {
            if (getOptions().isVerbose()) std::printf("Decoding tile #%d\n", curIndex);

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

            // creating new tile data object
            uint32_t chunkSize;
            if (fin.read(&v32, 4, 1) != 1) return false;
            chunkSize = get32u_le(&v32);
            if (chunkSize == 0) {
              std::printf("\nInvalid block size found for tile #%d\n", curIndex);
              return false;
            }
            BytePtr ptrIndexed(new uint8_t[MAX_TILE_SIZE_8], std::default_delete<uint8_t[]>());
            BytePtr ptrPalette(new uint8_t[PALETTE_SIZE], std::default_delete<uint8_t[]>());
            BytePtr ptrDeflated(new uint8_t[chunkSize], std::default_delete<uint8_t[]>());
            if (fin.read(ptrDeflated.get(), 1, chunkSize) != chunkSize) return false;
            TileDataPtr tileData(new TileData(getOptions()));
            tileData->setEncoding(false);
            tileData->setIndex(curIndex);
            tileData->setType(compType);
            tileData->setPaletteData(ptrPalette);
            tileData->setIndexedData(ptrIndexed);
            tileData->setDeflatedData(ptrDeflated);
            tileData->setSize(chunkSize);
            pool->addTileData(tileData);
          }
        }

        // retrieving the remaining tile data blocks in queue
        while (!pool->finished()) {
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
          pool->waitForResult();
        }
        if (getOptions().getVerbosity() == 1) std::printf("\n");

        if (nextTileIdx < tileCount) {
          std::printf("Missing tiles. Only %d of %d tiles converted.\n", nextTileIdx, tileCount);
          return false;
        }

        if (getOptions().isMosc()) {
          // compressing mos -> mosc
          uint32_t moscSize = mosSize*2;
          Compression compression;
          BytePtr moscData(new uint8_t[moscSize], std::default_delete<uint8_t[]>());

          // writing MOSC header
          std::memcpy(moscData.get(), HEADER_MOSC_SIGNATURE, 4);
          std::memcpy(moscData.get()+4, HEADER_VERSION_V1, 4);
          v32 = get32u_le(&mosSize);
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


bool Graphics::tizToTIS(const std::string &inFile, const std::string &outFile) noexcept
{
  if (!inFile.empty() && !outFile.empty() && inFile != outFile) {
    File fin(inFile.c_str(), "rb");
    if (!fin.error()) {
      char sig[4], tsig[4];
      uint32_t compType = Options::GetEncodingCode(Encoding::Z, false);
      uint32_t tileCount;
      uint16_t v16;

      // parsing TIZ header
      if (fin.read(sig, 1, 4) != 4) return false;;
      if (std::strncmp(sig, HEADER_TIZ_SIGNATURE, 4) != 0) {
        std::printf("Invalid TIZ signature\n");
        return false;
      }

      if (fin.read(&v16, 2, 1) != 1) return false;
      tileCount = get16u_be(&v16);
      if (tileCount == 0) {
        std::printf("No tiles found\n");
        return false;
      }

      // skipping 2 bytes
      if (fin.read(&v16, 2, 1) != 1) return false;

      File fout(outFile.c_str(), "wb");
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

        ThreadPoolPtr pool = createThreadPool(*this, getOptions().getThreads(), 64);
        uint32_t nextTileIdx = 0, curProgress = 0;
        for (uint32_t tileIdx = 0; tileIdx < tileCount; tileIdx++) {
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
        }

        // retrieving the remaining tile data blocks in queue
        while (!pool->finished()) {
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
          pool->waitForResult();
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
      char sig[4], tsig[4];
      uint32_t compType = Options::GetEncodingCode(Encoding::Z, false);
      uint32_t mosWidth, mosHeight;
      uint16_t v16;

      // parsing TIZ header
      if (fin.read(sig, 1, 4) != 4) return false;;
      if (std::strncmp(sig, HEADER_MOZ_SIGNATURE, 4) != 0) {
        std::printf("Invalid MOZ signature\n");
        return false;
      }

      if (fin.read(&v16, 2, 1) != 1) return false;
      mosWidth = get16u_be(&v16);
      if (mosWidth == 0) {
        std::printf("Invalid MOZ width\n");
        return false;
      }

      if (fin.read(&v16, 2, 1) != 1) return false;
      mosHeight = get16u_be(&v16);
      if (mosHeight == 0) {
        std::printf("Invalid MOZ height\n");
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
        std::memcpy(mosData.get()+headerOfs, HEADER_MOS_SIGNATURE, 4);
        headerOfs += 4;
        std::memcpy(mosData.get()+headerOfs, HEADER_VERSION_V1, 4);
        headerOfs += 4;
        v16 = mosWidth; v16 = get16u_le(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos width
        headerOfs += 2;
        v16 = mosHeight; v16 = get16u_le(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos height
        headerOfs += 2;
        v16 = mosCols; v16 = get16u_le(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos columns
        headerOfs += 2;
        v16 = mosRows; v16 = get16u_le(&v16);
        std::memcpy(mosData.get()+headerOfs, &v16, 2);     // writing mos rows
        headerOfs += 2;
        v32 = 0x40; v32 = get32u_le(&v32);
        std::memcpy(mosData.get()+headerOfs, &v32, 4);     // writing tile dimension
        headerOfs += 4;
        v32 = 0x18; v32 = get32u_le(&v32);
        std::memcpy(mosData.get()+headerOfs, &v32, 4);     // writing offset to palettes
        headerOfs += 4;

        if (getOptions().isVerbose()) {
          std::printf("Width: %d, height: %d, columns: %d, rows: %d, encoding: %d - %s\n",
                      mosWidth, mosHeight, mosCols, mosRows, compType, Options::GetEncodingName(compType).c_str());
        }
        if (getOptions().getVerbosity() == 1) std::printf("Converting");

        ThreadPoolPtr pool = createThreadPool(*this, getOptions().getThreads(), 64);
        uint32_t tileCount = mosCols * mosRows;
        uint32_t nextTileIdx = 0, curProgress = 0;
        int curIndex = 0;                       // the current tile index
        for (uint32_t row = 0; row < mosRows; row++) {
          for (uint32_t col = 0; col < mosCols; col++, curIndex++) {
            if (getOptions().isVerbose()) std::printf("Decoding tile #%d\n", curIndex);

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

            // creating new tile data object
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
              std::printf("\nInvalid header found in tile #%d\n", curIndex);
              return false;
            }
            TileDataPtr tileData(new TileData(getOptions()));
            tileData->setEncoding(false);
            tileData->setIndex(curIndex);
            tileData->setType(compType);
            tileData->setPaletteData(ptrPalette);
            tileData->setIndexedData(ptrIndexed);
            tileData->setDeflatedData(ptrDeflated);
            tileData->setSize(chunkSize);
            pool->addTileData(tileData);
          }
        }

        // retrieving the remaining tile data blocks in queue
        while (!pool->finished()) {
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
          pool->waitForResult();
        }
        if (getOptions().getVerbosity() == 1) std::printf("\n");

        if (nextTileIdx < tileCount) {
          std::printf("Missing tiles. Only %d of %d tiles converted.\n", nextTileIdx, tileCount);
          return false;
        }

        if (getOptions().isMosc()) {
          // compressing mos -> mosc
          uint32_t moscSize = mosSize*2;
          Compression compression;
          BytePtr moscData(new uint8_t[moscSize], std::default_delete<uint8_t[]>());

          // writing MOSC header
          std::memcpy(moscData.get(), HEADER_MOSC_SIGNATURE, 4);
          std::memcpy(moscData.get()+4, HEADER_VERSION_V1, 4);
          v32 = get32u_le(&mosSize);
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
          std::printf("MOZ file converted successfully.\n");
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
