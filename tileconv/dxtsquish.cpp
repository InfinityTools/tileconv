/*
 * Copyright (C) 2012   argent77
 *
 * This file is part of tileconv.
 *
 * tileconv is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tileconv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tileconv.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <cstring>
#include "dxtsquish.h"

const int DxtSquish::DEF_FLAGS = Dxt1 | ColourClusterFit | WeightColourByAlpha;
const int DxtSquish::MASK_DXT = Dxt1 | Dxt3 | Dxt5;


DxtSquish::DxtSquish() noexcept
: DxtBase()
{
  setFlags(DEF_FLAGS);
}


DxtSquish::DxtSquish(ColorFormat format) noexcept
: DxtBase(format, DEF_FLAGS)
{
}


DxtSquish::DxtSquish(ColorFormat format, int flags) noexcept
: DxtBase(format, flags)
{
}


DxtSquish::~DxtSquish() noexcept
{
}


bool DxtSquish::compressBlock(uint8_t *src, uint8_t *dst) noexcept
{
  uint8_t buffer[64]; // temp. buffer for a single 4x4 pixel block

  if (src != nullptr && dst != nullptr) {
    std::memcpy(buffer, src, 64);
    ReorderColors(buffer, 16, getColorFormat(), ColorFormat::ABGR);
    squish::Compress(buffer, dst, getFlags());
    return true;
  }
  return false;
}


/*
bool DxtSquish::decompressBlock(uint8_t *src, uint8_t *dst) noexcept
{
  if (src != nullptr && dst != nullptr) {
    squish::Decompress(dst, src, getFlags());
    DxtBase::ReorderColors(dst, 16, ColorFormat::ABGR, getColorFormat());
    return true;
  }
  return false;
}
*/


void DxtSquish::setDxt1() noexcept
{
  setFlags((getFlags() & ~MASK_DXT) | Dxt1);
}


void DxtSquish::setDxt3() noexcept
{
  setFlags((getFlags() & ~MASK_DXT) | Dxt3);
}


void DxtSquish::setDxt5() noexcept
{
  setFlags((getFlags() & ~MASK_DXT) | Dxt5);
}


bool DxtSquish::isDxt1() const noexcept
{
  return (getFlags() & Dxt1) != 0;
}


bool DxtSquish::isDxt3() const noexcept
{
  return (getFlags() & Dxt3) != 0;
}


bool DxtSquish::isDxt5() const noexcept
{
  return (getFlags() & Dxt5) != 0;
}

