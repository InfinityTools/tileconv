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
#ifndef _JPEG_H_
#define _JPEG_H_
#include <stdio.h>
#include <jpeglib.h>
#include "options.h"

namespace tc {

/** JPEG decompression routines for TIZ/MOZ tiles. */
class Jpeg
{
public:
  Jpeg(const Options& options) noexcept;
  ~Jpeg() noexcept;

  /** Read-only access to Options instance. */
  const Options& getOptions() const noexcept { return m_options; }

  /**
   * Decompress a JPEG image to an RGB or grayscale image.
   * \param srcBuf Buffer containing JPEG image data.
   * \param srcBufSize Size of the JPEG image in bytes.
   * \param srcPal A JSAMPARRAY[3] structure containing arrays of R, G and B color components.
   * \param dstPal Storage for decompressed palette data (256 ARGB entries)
   * \param dstBuf Storage for decompressed image data (required size depends on image dimensions).
   * \return Size of palette + decoded pixel data.
   */
  int decompress(uint8_t *srcBuf, size_t srcBufSize, JSAMPARRAY srcPal,
                 uint8_t *dstPal, uint8_t *dstBuf) noexcept;

  /**
   * Attempts to retrieve information about the JPEG image specfied in the given buffer.
   * Information can be queried with getWidth(), getHeight() and getSubSampling() afterwards.
   * \param buf Buffer holding the JPEG image.
   * \param bufSize Size of the buffer in bytes.
   * \return true if call is successful, false otherwise.
   */
  bool updateInformation(uint8_t *buf, size_t bufSize) noexcept;

  /** Retrieve width, height after a getInformation() call. */
  int getWidth() const noexcept { return m_width; }
  int getHeight() const noexcept { return m_height; }

private:
  // Custom error handling mechanism
  static void MyErrorExit(j_common_ptr cinfo);

  bool isError() const noexcept { return m_error; }
  void setError(bool b) noexcept { m_error = b; }

private:
  const Options&                m_options;
  struct jpeg_decompress_struct m_info;
  struct jpeg_error_mgr         m_err;
  int                           m_width;
  int                           m_height;
  bool                          m_error;
};

}   // namespace tc

#endif		// _JPEG_H_
