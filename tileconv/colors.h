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

namespace tc {

/** Provides functions for colorspace reduction and expansion. */
class Colors
{
public:
  Colors(const Options &options) noexcept;
  ~Colors() noexcept;

  /**
   * Converts a 8-bit paletted data block into a 32-bit ARGB data block.
   * \param src Data block containing 8-bit color indices.
   * \param palette A color table of 256 entries using ARGB component order.
   * \param dst Data block to store the resulting 32-bit ARGB pixel into. (Note: ARGB = {b, g, r, a, ...})
   * \param size Number of pixels in the source block and available space in the target block.
   * \return The number of converted pixels or 0 on error.
   */
  int palToARGB(uint8_t *src, uint8_t *palette, uint8_t *dst, uint32_t size) noexcept;

  /**
   * Converts a 32-bit ARGB data block into a 8-bit paletted data block.
   * \param src Data block containing 32-bit ARGB pixels. (Note: ARGB = {b, g, r, a, ...})
   * \param dst Data block to store the resulting 8-bit indices into.
   * \param palette A ARGB color table to store 256 entries into. (Note: ARGB = {b, g, r, a, ...})
   * \param width Image width in pixels.
   * \param height Image height in pixels.
   * \return The number of converted pixels or 0 on error.
   */
  int ARGBToPal(uint8_t *src, uint8_t *dst, uint8_t *palette, uint32_t width, uint32_t height) noexcept;

  /** Read-only access to Options structure. */
  const Options& getOptions() const noexcept { return m_options; }

private:
  const Options&    m_options;
};

}   // namespace tc

#endif
