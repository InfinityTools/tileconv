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
#include <cstdio>
#include <cstring>
#include "version.h"
#include "funcs.h"
#include "fileio.h"
#include "options.h"
#include "compress.h"
#include "graphics.h"
#include "tileconv.h"


int main(int argc, char *argv[])
{
  tc::TileConv tileConv(argc, argv);
  if (tileConv.execute()) {
    return 0;
  } else {
    return 1;
  }
}


namespace tc {

TileConv::TileConv() noexcept
: m_options()
, m_initialized(true)
{
}

TileConv::TileConv(int argc, char *argv[]) noexcept
: m_options()
{
  init(argc, argv);
}


TileConv::~TileConv() noexcept
{
}


bool TileConv::init(int argc, char *argv[]) noexcept
{
  return (m_initialized = m_options.init(argc, argv));
}


bool TileConv::execute() noexcept
{
  // checking state
  if (!isInitialized()) return false;
  if (getOptions().getInputCount() == 0) return false;

  // Special case: show information of input files only
  if (getOptions().isShowInfo()) {
    for (int i = 0; i < getOptions().getInputCount(); i++) {
      showInfo(getOptions().getInput(i));
      std::printf("\n");
    }
    return true;
  }

  // Starting the actual conversion process
  if (!getOptions().isSilent()) {
    std::printf("Options: %s\n", getOptions().getOptionsSummary(true).c_str());
  }

  Graphics gfx(m_options);
  bool retVal = true;
  for (int i = 0; i < getOptions().getInputCount(); i++) {
    if (!getOptions().isSilent() && getOptions().getInputCount() > 1) {
      std::printf("\nProcessing file %d of %d\n", i+1, getOptions().getInputCount());
    }
    const std::string &inputFile = getOptions().getInput(i);
    std::string outputFile;
    if (!inputFile.empty()) {
      switch (Options::GetFileType(inputFile, getOptions().assumeTis())) {
        case FileType::TIS:
          // generating output filename
          if (getOptions().isOutFile()) {
            outputFile = getOptions().getOutPath() + getOptions().getOutFile();
          } else {
            outputFile = getOptions().getOutPath() + Options::SetFileExt(inputFile, FileType::TBC);
          }
          if (outputFile.empty()) {
            retVal = false;
            std::printf("Error creating output filename\n");
            if (getOptions().isHaltOnError()) {
              return retVal;
            } else {
              break;
            }
          }
          // converting
          if (!getOptions().isSilent()) {
            std::printf("Converting TIS -> TBC\n");
            std::printf("Input: \"%s\", output: \"%s\"\n", inputFile.c_str(), outputFile.c_str());
          }
          if (!gfx.tisToTBC(inputFile, outputFile)) {
            retVal = false;
            std::printf("Error while converting \"%s\"\n", inputFile.c_str());
            if (getOptions().isHaltOnError()) {
              return retVal;
            }
          }
          break;
        case FileType::MOS:
          // generating output filename
          if (getOptions().isOutFile()) {
            outputFile = getOptions().getOutPath() + getOptions().getOutFile();
          } else {
            outputFile = getOptions().getOutPath() + Options::SetFileExt(inputFile, FileType::MBC);
          }
          if (outputFile.empty()) {
            retVal = false;
            std::printf("Error creating output filename\n");
            if (getOptions().isHaltOnError()) {
              return retVal;
            } else {
              break;
            }
          }
          // converting
          if (!getOptions().isSilent()) {
            std::printf("Converting MOS -> MBC\n");
            std::printf("Input: \"%s\", output: \"%s\"\n", inputFile.c_str(), outputFile.c_str());
          }
          if (!gfx.mosToMBC(inputFile, outputFile)) {
            retVal = false;
            std::printf("Error while converting \"%s\"\n", inputFile.c_str());
            if (getOptions().isHaltOnError()) {
              return retVal;
            }
          }
          break;
        case FileType::TBC:
          // generating output filename
          if (getOptions().isOutFile()) {
            outputFile = getOptions().getOutPath() + getOptions().getOutFile();
          } else {
            outputFile = getOptions().getOutPath() + Options::SetFileExt(inputFile, FileType::TIS);
          }
          if (outputFile.empty()) {
            retVal = false;
            std::printf("Error creating output filename\n");
            if (getOptions().isHaltOnError()) {
              return retVal;
            } else {
              break;
            }
          }
          // converting
          if (!getOptions().isSilent()) {
            std::printf("Converting TBC -> TIS\n");
            std::printf("Input: \"%s\", output: \"%s\"\n", inputFile.c_str(), outputFile.c_str());
          }
          if (!gfx.tbcToTIS(inputFile, outputFile)) {
            retVal = false;
            std::printf("Error while converting \"%s\"\n", inputFile.c_str());
            if (getOptions().isHaltOnError()) {
              return retVal;
            }
          }
          break;
        case FileType::MBC:
          // generating output filename
          if (getOptions().isOutFile()) {
            outputFile = getOptions().getOutPath() + getOptions().getOutFile();
          } else {
            outputFile = getOptions().getOutPath() + Options::SetFileExt(inputFile, FileType::MOS);
          }
          if (outputFile.empty()) {
            retVal = false;
            std::printf("Error creating output filename\n");
            if (getOptions().isHaltOnError()) {
              return retVal;
            } else {
              break;
            }
          }
          // converting
          if (!getOptions().isSilent()) {
            std::printf("Converting MBC -> MOS\n");
            std::printf("Input: \"%s\", output: \"%s\"\n", inputFile.c_str(), outputFile.c_str());
          }
          if (!gfx.mbcToMOS(inputFile, outputFile)) {
            retVal = false;
            std::printf("Error while converting \"%s\"\n", inputFile.c_str());
            if (getOptions().isHaltOnError()) {
              return retVal;
            }
          }
          break;
        default:
          retVal = false;
          std::printf("Unsupported file type: \"%s\"\n", inputFile.c_str());
          if (getOptions().isHaltOnError()) {
            return retVal;
          }
          break;
      }
    }
  }
  return retVal;
}


bool TileConv::showInfo(const std::string &fileName) noexcept
{
  if (!fileName.empty()) {
    std::printf("Parsing \"%s\"...\n", fileName.c_str());
    File f(fileName.c_str(), "rb");
    if (!f.error()) {
      char sig[4], ver[4];

      if (f.read(sig, 1, 4) != 4) return false;
      if (std::strncmp(sig, Graphics::HEADER_TIS_SIGNATURE, 4) == 0) {
        uint32_t tileNum, tileSize;
        // Parsing TIS file
        if (f.read(ver, 1, 4) != 4) return false;
        if (std::strncmp(ver, Graphics::HEADER_VERSION_V1, 4) != 0 &&
            std::strncmp(ver, Graphics::HEADER_VERSION_V2, 4) != 0) {
          std::printf("Invalid TIS version.\n");
          return false;
        }
        if (f.read(&tileNum, 4, 1) != 1) return false;
        tileNum = get32u(&tileNum);
        if (f.read(&tileSize, 4, 1) != 1) return false;
        tileSize = get32u(&tileSize);

        // Displaying TIS stats
        std::printf("File type:       TIS\n");
        switch (tileSize) {
          case 0x1400: std::printf("File subtype:    Paletted\n"); break;
          case 0x000c: std::printf("File subtype:    PVRZ-based\n"); break;
          default:     std::printf("File subtype:    Unknown\n"); break;
        }
        std::printf("Number of tiles: %d\n", tileNum);
      } else if (std::strncmp(sig, Graphics::HEADER_MOS_SIGNATURE, 4) == 0 ||
                 std::strncmp(sig, Graphics::HEADER_MOSC_SIGNATURE, 4) == 0) {
        // Parsing MOS or MOSC file
        BytePtr mosData(nullptr, std::default_delete<uint8_t[]>());
        bool isMosc = true;
        uint32_t mosSize;
        uint16_t width, height, cols, rows;
        if (f.read(ver, 1, 4) != 4) return false;

        if (std::strncmp(sig, Graphics::HEADER_MOSC_SIGNATURE, 4) == 0) {
          // decode MOSC into memory
          uint32_t moscSize;
          Compression compression;
          isMosc = true;

          // getting MOSC file size
          moscSize = f.getsize();
          if (moscSize <= 12) {
            std::printf("Invalid MOSC size\n");
            return false;
          }
          moscSize -= 12;    // removing header size
          f.seek(4, SEEK_SET);
          if (f.read(&ver, 1, 4) != 4) return false;
          if (std::strncmp(ver, Graphics::HEADER_VERSION_V1, 4) != 0) {
            std::printf("Invalid MOSC version.\n");
            return false;
          }
          if (f.read(&mosSize, 4, 1) != 1) return false;
          mosSize = get32u(&mosSize);
          if (mosSize < 24) {
            std::printf("MOS size too small.\n");
            return false;
          }
          // loading and decompressing MOS data
          BytePtr moscData(new uint8_t[moscSize], std::default_delete<uint8_t[]>());
          if (f.read(moscData.get(), 1, moscSize) < moscSize) {
            std::printf("Incomplete or corrupted MOSC file.\n");
            return false;
          }
          mosData.reset(new uint8_t[mosSize], std::default_delete<uint8_t[]>());
          uint32_t size = compression.inflate(moscData.get(), moscSize, mosData.get(), mosSize);
          if (size != mosSize) {
            std::printf("Error while decompressing MOSC input file.\n");
            return false;
          }
        } else {
          // reading MOS header into memory
          mosSize = f.getsize();
          if (mosSize < 24) {
            std::printf("MOS size too small\n");
            return false;
          }
          f.seek(0, SEEK_SET);
          mosData.reset(new uint8_t[24], std::default_delete<uint8_t[]>());
          if (f.read(mosData.get(), 1, 24) != 24) return false;
        }

        std::memcpy(ver, mosData.get()+0x04, 4);
        std::memcpy(&width, mosData.get()+0x08, 2);
        std::memcpy(&height, mosData.get()+0x0a, 2);
        std::memcpy(&cols, mosData.get()+0x0c, 2);
        std::memcpy(&rows, mosData.get()+0x0e, 2);

        // Displaying TIS stats
        std::printf("File type:       MOS (%s)\n", isMosc ? "compressed" : "uncompressed");
        if (std::strncmp(ver, "V1  ", 4) == 0) {
          std::printf("File subtype:    Paletted (V1)\n");
        } else if (std::strncmp(ver, "V2  ", 4) == 0) {
          std::printf("File subtype:    PVRZ-based (V2)\n");
        } else {
          std::printf("File version:    Unknown\n");
        }
        std::printf("Width:           %d\n", width);
        std::printf("Height:          %d\n", height);
        std::printf("Columns:         %d\n", cols);
        std::printf("Rows:            %d\n", rows);
      } else if (std::strncmp(sig, Graphics::HEADER_TBC_SIGNATURE, 4) == 0) {
        // Parsing TBC file
        uint32_t compType, tileNum;
        if (f.read(ver, 1, 4) != 4) return false;
        if (std::strncmp(ver, Graphics::HEADER_VERSION_V1_0, 4) != 0) {
          std::printf("Invalid or unsupported TBC version.\n");
          return false;
        }
        if (f.read(&compType, 4, 1) != 1) return false;
        compType = get32u(&compType);
        if (f.read(&tileNum, 4, 1) != 1) return false;
        tileNum = get32u(&tileNum);

        // Displaying TBC stats
        std::printf("File type:       TBC\n");
        std::printf("TBC version:     1.0\n");
        std::printf("Compression:     0x%04x - %s\n", compType, Options::GetEncodingName(compType).c_str());
        std::printf("Number of tiles: %d\n", tileNum);
      } else if (std::strncmp(sig, Graphics::HEADER_MBC_SIGNATURE, 4) == 0) {
        // Parsing MBC file
        uint32_t compType, width, height, tileNum;
        if (f.read(ver, 1, 4) != 4) return false;
        if (std::strncmp(ver, Graphics::HEADER_VERSION_V1_0, 4) != 0) {
          std::printf("Invalid or unsupported MBC version.\n");
          return false;
        }
        if (f.read(&compType, 4, 1) != 1) return false;
        compType = get32u(&compType);
        if (f.read(&width, 4, 1) != 1) return false;
        width = get32u(&width);
        if (f.read(&height, 4, 1) != 1) return false;
        height = get32u(&height);

        tileNum = (((width+63) & 63)*((height+63) & 63)) / 4096;

        // Displaying TBC stats
        std::printf("File type:       MBC\n");
        std::printf("MBC version:     1.0\n");
        std::printf("Compression:     0x%04x - %s\n", compType, Options::GetEncodingName(compType).c_str());
        std::printf("Width:           %d\n", width);
        std::printf("Height:          %d\n", height);
        std::printf("Number of tiles: %d\n", tileNum);
      } else if (getOptions().assumeTis() && (f.getsize() % 5120) == 0) {
        int size = (int)f.getsize();
        int tileNum = size / 5120;

        // Displaying assumed TIS stats
        std::printf("File type:       TIS (assumed)\n");
        std::printf("File subtype:    Paletted\n");
        std::printf("Number of tiles: %d\n", tileNum);
      } else {
        std::printf("File type:       Unknown\n");
        return false;
      }
      return true;
    }
  }
  return false;
}

}   // namespace tc
