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
#include <algorithm>
#include <cstring>
#include "funcs.h"
#include "converterfactory.h"
#include "compress.h"
#include "tiledata.h"

namespace tc {

const unsigned TileData::PALETTE_SIZE      = 1024;
const unsigned TileData::MAX_TILE_SIZE_8   = 64*64;
const unsigned TileData::MAX_TILE_SIZE_32  = 64*64*4;


TileData::TileData(const Options &options) noexcept
: m_options(options)
, m_encoding(false)
, m_error(false)
, m_ptrPalette(nullptr)
, m_ptrIndexed(nullptr)
, m_ptrDeflated(nullptr)
, m_index(-1)
, m_width(0)
, m_height(0)
, m_type(0)
, m_size(0)
, m_errorMsg()
{
}


TileData::~TileData() noexcept
{
}


TileData& TileData::operator()() noexcept
{
  if (isEncoding()) {
    encode();
  } else {
    decode();
  }
  return *this;
}


void TileData::setIndex(int index) noexcept
{
  m_index = std::max(0, std::min(std::numeric_limits<int>::max(), index));
}


void TileData::setWidth(int width) noexcept
{
  m_width = std::max(0, std::min(std::numeric_limits<int>::max(), width));
}


void TileData::setHeight(int height) noexcept
{
  m_height = std::max(0, std::min(std::numeric_limits<int>::max(), height));
}


void TileData::setType(unsigned type) noexcept
{
  m_type = std::max(0u, std::min(std::numeric_limits<unsigned>::max(), type));
}


void TileData::setSize(int size) noexcept
{
  m_size = std::max(0, std::min(std::numeric_limits<int>::max(), size));
}


bool TileData::isValid() const noexcept
{
  if (isEncoding()) {
    return (m_ptrPalette != nullptr && m_ptrIndexed != nullptr && m_ptrDeflated != nullptr &&
            m_index >= 0 && m_width > 0 && m_height > 0);
  } else {
    return (m_ptrPalette != nullptr && m_ptrIndexed != nullptr && m_ptrDeflated != nullptr &&
            m_index >= 0 && m_size > 0);
  }
}


void TileData::encode() noexcept
{
  if (isValid()) {
    ConverterPtr converter =
        ConverterFactory::GetConverter(getOptions(),
                                       Options::GetEncodingCode(getOptions().getEncoding(),
                                                                getOptions().isDeflate()));

    if (converter != nullptr) {
      converter->setEncoding(true);
      converter->setColorFormat(Converter::ColorFormat::ARGB);

      unsigned tileSizeEncoded = converter->getRequiredSpace(getWidth(), getHeight()) + HEADER_TILE_ENCODED_SIZE;
      if (tileSizeEncoded <= HEADER_TILE_ENCODED_SIZE) {
        setError(true);
        setErrorMsg("Error while calculating space\n");
        return;
      }
      BytePtr  ptrEncoded(new uint8_t[tileSizeEncoded], std::default_delete<uint8_t[]>());
      uint16_t v16;
      setSize(0);

      uint8_t *encodedPtr = ptrEncoded.get();
      v16 = (uint16_t)getWidth(); v16 = get16u(&v16);    // tile width in ready-to-write format
      *((uint16_t*)encodedPtr) = v16; encodedPtr += 2;   // setting tile width
      v16 = (uint16_t)getHeight(); v16 = get16u(&v16);   // tile height in ready-to-write format
      *((uint16_t*)encodedPtr) = v16; encodedPtr += 2;   // setting tile height

      if (!converter->convert(getPaletteData().get(), getIndexedData().get(), encodedPtr,
                              getWidth(), getHeight())) {
        setError(true);
        setErrorMsg("Error while encoding tile data\n");
        return;
      }

      if (getOptions().isDeflate()) {
        // applying zlib compression
        Compression compression;
        setSize(compression.deflate(ptrEncoded.get(), tileSizeEncoded,
                                    getDeflatedData().get(), tileSizeEncoded*2));
        if (getSize() == 0) {
          setError(true);
          setErrorMsg("Error while compressing tile data\n");
          return;
        }
      } else {
        // using pixel encoding only
        setSize(tileSizeEncoded);
        std::memcpy(getDeflatedData().get(), ptrEncoded.get(), getSize());
      }
    } else {
      setError(true);
      setErrorMsg("Unsupported source format found\n");
    }
  } else {
    setError(true);
    setErrorMsg("Invalid tile data found\n");
  }
}


void TileData::decode() noexcept
{
  if (isValid()) {
    ConverterPtr converter = ConverterFactory::GetConverter(getOptions(), getType());

    if (converter != nullptr) {
      converter->setEncoding(false);
      converter->setColorFormat(Converter::ColorFormat::ARGB);

      BytePtr  ptrEncoded(new uint8_t[MAX_TILE_SIZE_32], std::default_delete<uint8_t[]>());

      if (Options::IsTileDeflated(getType())) {
        // inflating zlib compressed data
        Compression compression;
        compression.inflate(getDeflatedData().get(), getSize(), ptrEncoded.get(), MAX_TILE_SIZE_32);
      } else {
        // copy pixel encoded tile data
        std::memcpy(ptrEncoded.get(), getDeflatedData().get(), getSize());
      }

      setSize(0);

      // retrieving tile dimensions
      setWidth(get16u((uint16_t*)ptrEncoded.get()));
      setHeight(get16u((uint16_t*)(ptrEncoded.get()+2)));

      setSize(converter->convert(getPaletteData().get(), getIndexedData().get(),
                                 ptrEncoded.get() + HEADER_TILE_ENCODED_SIZE,
                                 getWidth(), getHeight()));
      if (getSize() == 0) {
        setError(true);
        setErrorMsg("Error while decoding tile data\n");
        return;
      }
    } else {
      setError(true);
      setErrorMsg("Unsupported source format found\n");
    }
  } else {
    setError(true);
    setErrorMsg("Invalid tile data found\n");
  }
}

}   // namespace tc

