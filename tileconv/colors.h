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
#ifndef COLORS_H
#define COLORS_H
#include <unordered_map>
#include "options.h"

/** Provides functions for colorspace reduction and expansion. */
class Colors
{
public:
  // Available color component orders
  enum ColorFormat {
    FMT_ARGB, FMT_ABGR, FMT_BGRA, FMT_RGBA
  };

public:
  Colors(const Options &options) noexcept;
  ~Colors() noexcept;

  /**
   * Converts a 8-bit paletted data block into a 32-bit ARGB data block.
   * \param src Data block containing 8-bit color indices.
   * \param palette A color table of 256 entries.
   * \param dst Data block to store the resulting 32-bit ARGB pixel into. (Note: ARGB = {b, g, r, a, ...})
   * \param size Number of pixels in the source block and available space in the target block.
   * \return The number of converted pixels or 0 on error.
   */
  uint32_t palToARGB(uint8_t *src, uint8_t *palette, uint8_t *dst, uint32_t size) noexcept;

  /**
   * Converts a 32-bit ARGB data block into a 8-bit paletted data block.
   * \param src Data block containing 32-bit ARGB pixels. (Note: ARGB = {b, g, r, a, ...})
   * \param dst Data block to store the resulting 8-bit indices into.
   * \param palette A ARGB color table to store 256 entries into. (Note: ARGB = {b, g, r, a, ...})
   * \param width Image width in pixels.
   * \param height Image height in pixels.
   * \return The number of converted pixels or 0 on error.
   */
  uint32_t ARGBToPal(uint8_t *src, uint8_t *dst, uint8_t *palette, uint32_t width, uint32_t height) noexcept;

  /**
   * Returns the value expanded to a multiple of 4.
   */
  uint32_t getPaddedValue(uint32_t v) const noexcept { return (v + 3) & 0xfffffffcu; }

  /**
   * Expands the source data block to have dimensions of a multiple of 4.
   * \param src The source data block containing 32-bit pixels.
   * \param dst The target data block to store the padded pixels into.
   * \param width The width of the source data block in pixels.
   * \param height The height of the source data block in pixels.
   * \param newWidth The width of the target data block in pixels (must be a multiple of 4 and equal to or greater than width!).
   * \param newHeight The height of the target data block in pixels (must be a multiple of 4 and equal to or greater than height!).
   * \return The resulting number of pixels stored into the target data block or 0 on error.
   */
  uint32_t padBlock(uint8_t *src, uint8_t *dst, unsigned width, unsigned height, unsigned newWidth, unsigned newHeight) noexcept;

  /**
   * Shrinks the source data block to the actual size.
   * \param src The source data block containing padded 32-bit pixels to a multiple of 4.
   * \param dst The target data block to store the unpadded pixels into.
   * \param width The source block width (must be a multiple of 4!).
   * \param height The source block height (must be a multiple of 4!).
   * \param newWidth The target block width (must be less or equal to width!).
   * \param newHeight The target block height (must be less or equal to height!).
   * \return The resulting number of pixels stored into the target data block or 0 on error.
   */
  uint32_t unpadBlock(uint8_t *src, uint8_t *dst, unsigned width, unsigned height, unsigned newWidth, unsigned newHeight) noexcept;

  /**
   * Reorders the layout of the color components in a 32-bit pixel.
   * \param buffer The data block containing 32-bit pixels.
   * \param size The number of pixels available in buffer.
   * \param from The given color order.
   * \param to The resulting color order.
   * \return Number of converted pixels or 0 on error.
   */
  uint32_t reorderColors(uint8_t *buffer, uint32_t size, ColorFormat from, ColorFormat to) noexcept;

  /** Read-only access to Options structure. */
  const Options& getOptions() const noexcept { return m_options; }

private:
  const Options&    m_options;
};

#endif
