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
#ifndef _COLORQUANT_H_
#define _COLORQUANT_H_

#include <lib/libimagequant.h>

namespace tc {

class ColorQuant
{
public:
  ColorQuant() noexcept;
  ~ColorQuant() noexcept;

  /**
   * Set the original 32-bit ABGR bitmap.
   * \param bitmap The bitmap pixel data as a contiguous 32-bit ABGR memory block.
   * \param width The bitmap width in pixels.
   * \param height The bitmap height in pixels.
   * \param gamma An optional gamma value (defined as 1/gamma!). Default: 0
   * \return true for a valid bitmap, false otherwise.
   */
  bool setSource(void *bitmap, int width, int height, double gamma = 0.0) noexcept;
  void* getSource() const noexcept { return m_source; }

  /**
   * Set the target buffer to store the resulting indexed bitmap into. Should be set after
   * calling setSource() for a meaningful error check.
   * \param buffer The buffer to store the indexed pixel data into.
   * \param size The size of the buffer.
   * \return true if the buffer is valid, false otherwise.
   */
  bool setTarget(void *buffer, int size) noexcept;
  void* getTarget() const noexcept { return m_target; }

  /**
   * Set the storage buffer for the resulting palette.
   * \param palette The palette data buffer. A 256 color palette occupies 1024 bytes of space.
   * \param size The size of the palette in bytes.
   * \return true if the palette buffer is valid, false otherwise.
   */
  bool setPalette(void *palette, int size) noexcept;
  void* getPalette() const noexcept { return m_palette; }

  /**
   * Executes the quantization process.
   * Note: You must specify source, target and palette beforehand.
   * \return true if quantization proceeded successfully, false otherwise.
   */
  bool quantize() noexcept;

  /** Define whether to use dithering. Default: Speed dependent. */
  void setDithering(bool b) noexcept { m_dithering = b; }
  bool isDithering() const noexcept { return m_dithering; }

  /**
   * Set the max. number of colors to use. Range: [2..256], Default: 256
   * Note: It's preferred to use setQuality() instead.
   */
  void setMaxColors(int colors) noexcept;
  int getMaxColors() const noexcept { return m_maxColors; }

  /** Set min/max quality between [0..100]. Default: min=0, max= 100 */
  void setQuality(int min, int max) noexcept;
  int getQualityMin() noexcept { return m_qualityMin; }
  int getQualityMax() noexcept { return m_qualityMax; }

  /** Uses different quantization features, depending on this value. Range: [1..10], Default: 3 */
  void setSpeed(int speed) noexcept;
  int getSpeed() const noexcept { return m_speed; }

  /** Alpha values higher than the given value will be assumed as opaque. Range: [0..255], Default: 255 */
  void setMinOpacity(int min) noexcept;
  int getMinOpacity() const noexcept { return m_minOpacity; }

  /**
   * Ignores given number of significant bits in all channels. 0 gives full quality.
   * Use 2 for RGB565, 3 for RGB555 or 4 for RGB444/RGBA4444. Range: [0..4], Default: 0
   */
  void setPosterization(int bits) noexcept;
  int getPosterization() const noexcept { return m_posterization; }

  /** Enable to set the transparent color index to the last palette position. Default: false */
  void setTransparentIndexLast(bool b) noexcept { m_lastTransparent = b; }
  bool isTransparentIndexLast() const noexcept { return m_lastTransparent; }

  /**
   * Returns the mean square error of quantization of the last quantization operation.
   * For most images MSE 1..5 is excellent, 7-10 is ok, 20-30 is average and 100 is awful.
   * Returns negative value on error.
   */
  double getQuantizationError() noexcept;

  /** Returns the quantization error as a quality value in range [0..100]. Returns negative value on error. */
  double getQuantizationQuality() noexcept;

private:
  // frees memory of internal objects
  void freeMemory() noexcept;

private:
  bool      m_dithering;        // dithering enabled/disabled (depends on m_speed)
  bool      m_lastTransparent;  // set transparent palette index last (false)
  int       m_maxColors;        // max. number of colors to create (256)
  int       m_qualityMin;       // min. quality to accept (0)
  int       m_qualityMax;       // max. quality to accept (100)
  int       m_speed;            // enables features depending on this value (3)
  int       m_minOpacity;       // alpha value of above this value will be made opaque (255)
  int       m_posterization;    // how many bits to discard when quantizing colors (0)
  void      *m_source;          // 32-bit ABGR source bitmap
  int       m_width;            // bitmap width in pixels
  int       m_height;           // bitmap height in pixels
  double    m_gamma;            // bitmap gamma used for distributing color in light/dark areas (0.0)
  void      *m_target;          // storage for 8-bit indexed target bitmap
  unsigned  m_targetSize;       // size of target buffer in bytes
  void      *m_palette;         // buffer for resulting palette data (uses 4 bytes per color entry)
  unsigned  m_paletteSize;      // size of the palette buffer in bytes

  liq_attr    *m_liqAttr;     // internally used, stores quantizatin options
  liq_image   *m_liqImage;    // internally used, stores image data
  liq_result  *m_liqResult;   // internally used, stores quantization data
};

}   // namespace tc

#endif		// _COLORQUANT_H_
