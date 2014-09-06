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
#ifndef _DXTSQUISH_H_
#define _DXTSQUISH_H_
#include <squish.h>
#include "dxtbase.h"

/** Encapsulated libsquish implementation of DXTn compression and decompression. */
class DxtSquish : public DxtBase
{
public:
  /** Recreated libsquish flags */
  enum {
    Dxt1 = squish::kDxt1,
    Dxt3 = squish::kDxt3,
    Dxt5 = squish::kDxt5,
    ColourIterativeClusterFit = squish::kColourIterativeClusterFit,
    ColourClusterFit = squish::kColourClusterFit,
    ColourRangeFit = squish::kColourRangeFit,
    WeightColourByAlpha = squish::kWeightColourByAlpha
  };

public:
  DxtSquish() noexcept;
  DxtSquish(ColorFormat format) noexcept;
  DxtSquish(ColorFormat format, int flags) noexcept;
  ~DxtSquish() noexcept;

  /** See DxtBase::compressBlock() */
  bool compressBlock(uint8_t *src, uint8_t *dst) noexcept;

  /** See DxtBase::decompressBlock() */
//  bool decompressBlock(uint8_t *src, uint8_t *dst) noexcept;

  /** See DxtBase::setDxtn() */
  void setDxt1() noexcept;
  void setDxt3() noexcept;
  void setDxt5() noexcept;

  /** See DxtBase::isDxtn() */
  bool isDxt1() const noexcept;
  bool isDxt3() const noexcept;
  bool isDxt5() const noexcept;

private:
  static const int  DEF_FLAGS;
  static const int  MASK_DXT;
};



#endif		// _DXTSQUISH_H_
