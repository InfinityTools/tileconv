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
#ifndef _DXTBASE_H_
#define _DXTBASE_H_
#include <cstdint>
#include <memory>


/** Base class for different libraries providing DXTn compression and decompression functionality. */
class DxtBase
{
public:
  /** Available ARGB color formats. (Example: ARGB = 0xaarrggbb) */
  enum class ColorFormat { ARGB, ABGR, BGRA, RGBA };

public:
  virtual ~DxtBase() noexcept;

  /**
   * Compress a single DXTn block.
   * \param src Contains the 4x4 32-bit pixel block. Specify color format beforehand.
   * \param dst Storage space for the encoded DXTn data. (8 bytes for DXT1, 16 bytes for DXT3/DXT5)
   * \return true on success, false otherwise.
   */
  virtual bool compressBlock(uint8_t *src, uint8_t *dst) noexcept = 0;

  /**
   * Decompress a single DXTn block.
   * \param src The DXTn encoded data block.
   * \param dst Storage space for the decoded 4x4 32-bit pixel block. Specify color format beforehand.
   * \return true on success, false otherwise.
   */
  virtual bool decompressBlock(uint8_t *src, uint8_t *dst) noexcept;

  /**
   * Compress a whole image into DXTn data.
   * \param src The source image of 32-bit pixels. Specify color format beforehand.
   * \param dst Storage space for the encoded DXTn data.
   * \param width Width of the image in pixels.
   * \param height Height of the image in pixels.
   * \return true on success, false otherwise.
   */
  virtual bool compressImage(uint8_t *src, uint8_t *dst, unsigned width, unsigned height) noexcept;

  /**
   * Decompress DXTn encoded data into 32-bit pixel data.
   * \param src The DXTn encoded image data.
   * \param dst Storage space for the uncompressed image. Specify color format beforehand.
   * \param width Width of the image in pixels.
   * \param height Height of the image in pixels.
   * \return true on success, false otherwise.
   */
  virtual bool decompressImage(uint8_t *src, uint8_t *dst, unsigned width, unsigned height) noexcept;


  /** Returns the value expanded to the nearest multiple of 4. */
  unsigned getPaddedValue(unsigned v) const noexcept { return (v + 3) & ~3; }

  /**
   * Dimensions of a pixel data block will be expanded to the specified dimensions.
   * \param src Source block containing 32-bit pixels.
   * \param dst Storage for resulting pixel block after expanding.
   * \param srcWidth Width of source pixel block.
   * \param srcHeight Height of source pixel block.
   * \param dstWidth Resulting width of target pixel block. Must be >= srcWidth!
   * \param dstHeight Resulting height of target pixel block. Must be >= srcHeight!
   * \param useCopy If true, uses previous pixel values for expanded pixel data,
   *                use transparency/black otherwise.
   * \return Number of resulting pixels in dst, or 0 on error.
   */
  unsigned padBlock(const uint8_t *src, uint8_t *dst, unsigned srcWidth, unsigned srcHeight,
                    unsigned dstWidth, unsigned dstHeight, bool useCopy) noexcept;

  /**
   * Dimensions of a pixel data block will be reduced to the specified dimensions.
   * \param src Source block containing 32-bit pixels.
   * \param dst Storage for resulting pixel block after reduction.
   * \param srcWidth Width of source pixel block.
   * \param srcHeight Height of source pixel block.
   * \param dstWidth Resulting width of target pixel block. Must be <= srcWidth!
   * \param dstHeight Resulting height of target pixel block. Must be <= srcHeight!
   * \return Number of resulting pixels in dst, or 0 on error.
   */
  unsigned unpadBlock(const uint8_t *src, uint8_t *dst, unsigned srcWidth, unsigned srcHeight,
                      unsigned dstWidth, unsigned dstHeight) noexcept;

  /** The color format assumed for input data of compressBlock() and output data of decompressBlock(). */
  void setColorFormat(ColorFormat fmt) noexcept { m_colorFormat = fmt; }
  ColorFormat getColorFormat() const noexcept { return m_colorFormat; }

  /** Flags for controlling encoding type and (optional) quality. (Implementation-specific) */
  void setFlags(int flags) noexcept { m_flags = flags; }
  int getFlags() const noexcept { return m_flags; }

  /** Helper methods to set a specific DXT type. */
  virtual void setDxt1() noexcept = 0;
  virtual void setDxt3() noexcept = 0;
  virtual void setDxt5() noexcept = 0;

  /** Helper methods to query the currently selected encoding type. */
  virtual bool isDxt1() const noexcept = 0;
  virtual bool isDxt3() const noexcept = 0;
  virtual bool isDxt5() const noexcept = 0;

  /**
   * Returns the required space of the currently selected pixel encoding type for the given
   * image dimensions, or 0 on error.
   * (To be expand in derived classes if needed.)
   */
  virtual int getRequiredSpace(int width, int height) const noexcept;

  /**
   * Reorders the components in-place from one color format into another.
   * \param buffer The buffer containing 32-bit pixels in "from" order.
   * \param size The number of pixels in the buffer.
   * \param from The source color format.
   * \param to The target color format.
   * \return Success state.
   */
  static bool ReorderColors(uint8_t *buffer, unsigned size, ColorFormat from, ColorFormat to) noexcept;

protected:
  /** Initialize new instance, setting ARGB pixel format as default. */
  DxtBase() noexcept;
  DxtBase(ColorFormat format) noexcept;
  DxtBase(ColorFormat format, int flags) noexcept;

  // tileconv's own decoding routines as fallback solution. Decodes into ColorFormat::ARGB.
  static bool DecodeBlockDxt1(uint8_t *src, uint8_t *dst) noexcept;
  static bool DecodeBlockDxt3(uint8_t *src, uint8_t *dst) noexcept;
  static bool DecodeBlockDxt5(uint8_t *src, uint8_t *dst) noexcept;

  // Unpacks two consecutive 16-bit RGB565 colors into 32-bit ColorFormat::ARGB.
  static bool UnpackColors565(const uint8_t *src, uint8_t *dst) noexcept;

private:
  ColorFormat   m_colorFormat;
  int           m_flags;
};


/** Smart pointer type for DxtBase and derived classes. */
typedef std::shared_ptr<DxtBase>  DxtPtr;


#endif		// _DXTBASE_H_
