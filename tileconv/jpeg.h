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
#include <string>
#include <turbojpeg.h>

namespace tc {

/**
 * Encapsulates TurboJPEG decompression routines for ease of use.
 * See
 * http://svn.code.sf.net/p/libjpeg-turbo/code/branches/1.3.x/doc/html/group___turbo_j_p_e_g.html
 * for more information.
 */
class Jpeg
{
public:
  /** TurboJPEG Chrominance subsampling options */
  enum SubSampling {
    SAMP_444 = TJSAMP_444,
    SAMP_422 = TJSAMP_422,
    SAMP_420 = TJSAMP_420,
    SAMP_GRAY = TJSAMP_GRAY,
    SAMP_440 = TJSAMP_440
  };

  /** TurboJPEG pixel formats */
  enum PixelFormat {
    PF_RGB = TJPF_RGB,    // { r, g, b, ... }
    PF_BGR = TJPF_BGR,    // { b, g, r, ... }
    PF_RGBX = TJPF_RGBX,  // { r, g, b, x, ... }
    PF_BGRX = TJPF_BGRX,  // { b, g, r, x, ... }
    PF_XBGR = TJPF_XBGR,  // { x, b, g, r, ... }
    PF_XRGB = TJPF_XRGB,  // { x, r, g, b, ... }
    PF_GRAY = TJPF_GRAY,  // { g, ... }
    PF_RGBA = TJPF_RGBA,  // { r, g, b, a, ... } (alpha is always 255)
    PF_BGRA = TJPF_BGRA,  // { b, g, r, a, ... } (alpha is always 255)
    PF_ABGR = TJPF_ABGR,  // { a, b, g, r, ... } (alpha is always 255)
    PF_ARGB = TJPF_ARGB   // { a, r, g, b, ... } (alpha is always 255)
  };

  /** TurboJPEG flags for decompression. */
  enum Flags {
    FLAG_BOTTOMUP = TJFLAG_BOTTOMUP,      // mirror horizontally
    FLAG_FORCEMMX = TJFLAG_FORCEMMX,      // use MMX if available
    FLAG_FORCESSE = TJFLAG_FORCESSE,      // use SSE if available
    FLAG_FORCESSE2 = TJFLAG_FORCESSE2,    // use SSE2 if available
    FLAG_FORCESSE3 = TJFLAG_FORCESSE3,    // use SSE3 if available
    FLAG_FASTUPSAMPLE = TJFLAG_FASTUPSAMPLE // do not interpolate chroma when upsampling
  };

public:
  Jpeg() noexcept;
  ~Jpeg() noexcept;

  /**
   * Decompress a JPEG image to an RGB or grayscale image.
   * \param srcBuf Buffer containing JPEG image data.
   * \param srcBufSize Size of the JPEG image in bytes.
   * \param dstBuf Storage for decompressed image data (required size depends on pixel format).
   * \param pf Pixel format of the decompressed image.
   * \param flags Additional flags.
   * \return
   */
  bool decompress(uint8_t *srcBuf, size_t srcBufSize, uint8_t *dstBuf,
                  PixelFormat pf, int flags) noexcept;

  /**
   * Attempts to retrieve information about the JPEG image specfied in the given buffer.
   * Information can be queried with getWidth(), getHeight() and getSubSampling() afterwards.
   * \param buf Buffer holding the JPEG image.
   * \param bufSize Size of the buffer in bytes.
   * \return true if call is successful, false otherwise.
   */
  bool updateInformation(uint8_t *buf, size_t bufSize) noexcept;

  /** Retrieve width, height amd subsampling type after a getInformation() call. */
  int getWidth() const noexcept { return m_width; }
  int getHeight() const noexcept { return m_height; }
  SubSampling getSubSampling() const noexcept { return (SubSampling)m_subSampling; }

  /** Returns true if the last function call triggered an error. */
  bool isError() const noexcept { return m_error; }
  /** Returns a descriptive error message if the last function call failed. */
  std::string getErrorMsg() const noexcept;

private:
  // Called internally after each TurboJPEG function call
  void updateError(bool err) noexcept { m_error = err; }

private:
  tjhandle  m_handle;
  bool      m_error;
  int       m_width;
  int       m_height;
  int       m_subSampling;
};

}   // namespace tc

#endif		// _JPEG_H_
