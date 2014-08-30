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
#include "version.h"
#include "fileio.h"
#include "graphics.h"
#include "convert.h"


Convert::Convert() noexcept
: m_options()
{
}

Convert::Convert(int argc, char *argv[]) noexcept
: m_options()
{
  init(argc, argv);
}


Convert::~Convert() noexcept
{
}


bool Convert::init(int argc, char *argv[]) noexcept
{
  return m_options.init(argc, argv);
}


bool Convert::execute() noexcept
{
  // checking state
  if (getOptions().getInputCount() == 0) return false;
  if (!getOptions().isSilent()) {
    std::printf("Options: %s\n", getOptions().getOptionsSummary(true).c_str());
  }

  Graphics gfx(m_options);
  bool retVal = true;
  for (int i = 0; i < getOptions().getInputCount(); i++) {
    if (!getOptions().isSilent() && getOptions().getInputCount() > 1) {
      std::printf("Processing file %d of %d\n", i+1, getOptions().getInputCount());
    }
    const std::string &inputFile = getOptions().getInput(i);
    std::string outputFile;
    if (!inputFile.empty()) {
      switch (Options::GetFileType(inputFile)) {
        case FileType::TIS:
          // generating output filename
          if (!getOptions().isOutput()) {
            outputFile = Options::SetFileExt(inputFile, FileType::TBC);
          } else {
            outputFile = getOptions().getOutput();
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
          if (!getOptions().isOutput()) {
            outputFile = Options::SetFileExt(inputFile, FileType::MBC);
          } else {
            outputFile = getOptions().getOutput();
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
          if (!getOptions().isOutput()) {
            outputFile = Options::SetFileExt(inputFile, FileType::TIS);
          } else {
            outputFile = getOptions().getOutput();
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
          if (!getOptions().isOutput()) {
            outputFile = Options::SetFileExt(inputFile, FileType::MOS);
          } else {
            outputFile = getOptions().getOutput();
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

