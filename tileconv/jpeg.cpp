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
#include "jpeg.h"

namespace tc {

Jpeg::Jpeg(const Options& options) noexcept
: m_options(options)
, m_info()
, m_err()
, m_width()
, m_height()
{
  static_assert(JPEG_LIB_VERSION >= 80, "jpeg-turbo must be compiled with v8 API/ABI support.");

  m_info.err = jpeg_std_error(&m_err.pub);
  m_err.pub.error_exit = MyErrorExit;
  jpeg_create_decompress(&m_info);
}


Jpeg::~Jpeg() noexcept
{
  jpeg_destroy_decompress(&m_info);
}


int Jpeg::decompress(uint8_t *srcBuf, size_t srcBufSize, JSAMPARRAY srcPal,
                     uint8_t *dstPal, uint8_t *dstBuf) noexcept
{
  if (srcBuf != nullptr && srcBufSize > 0 && dstPal != nullptr && dstBuf != nullptr) {
    if (setjmp(m_err.setjmp_buffer)) {
      // handling jpeg errors
      jpeg_abort_decompress(&m_info);
      return 0;
    }

    // initializing decompression
    jpeg_mem_src(&m_info, srcBuf, srcBufSize);
    jpeg_read_header(&m_info, TRUE);
    m_width = m_info.image_width;
    m_height = m_info.image_height;

    // setting 8-bit mode
    m_info.quantize_colors = TRUE;
    if (getOptions().getDecodingQuality() >= 5) {
      m_info.dither_mode = JDITHER_FS;
    } else {
      m_info.dither_mode = JDITHER_NONE;
    }
    if (srcPal != nullptr) {
      // using predefined palette
      JSAMPLE *jpgPal[3];
      jpgPal[0] = srcPal[0] + 1;
      jpgPal[1] = srcPal[1] + 1;
      jpgPal[2] = srcPal[2] + 1;
      m_info.actual_number_of_colors = 255;
      m_info.colormap = (JSAMPARRAY)jpgPal;
    } else {
      // auto-generating palette
      m_info.desired_number_of_colors = 256;
      m_info.two_pass_quantize = TRUE;
      m_info.colormap = nullptr;
    }

    // start decompressing
    jpeg_start_decompress(&m_info);
    int tileSize = m_info.output_width*m_info.output_height;
    int len = 0;
    JSAMPARRAY buffer = (*m_info.mem->alloc_sarray)((j_common_ptr)&m_info, JPOOL_IMAGE, tileSize, 1);
    while (m_info.output_scanline < m_info.output_height) {
      jpeg_read_scanlines(&m_info, buffer, 1);
      memcpy(dstBuf + len, buffer[0], m_info.output_width);
      len += m_info.output_width;
    }

    if (srcPal != nullptr) {
      // storing predefined palette
      for (int i = 0; i < 256; i++) {
        dstPal[i << 2]       = srcPal[0][i];
        dstPal[(i << 2) + 1] = srcPal[1][i];
        dstPal[(i << 2) + 2] = srcPal[2][i];
        dstPal[(i << 2) + 3] = 0;
      }
      // adjusting for alpha
      for (int i = 0; i < tileSize; i++) {
        *dstBuf++ += 1;
      }
    } else {
      // checking colormap
      if (m_info.colormap != nullptr) {
        if (m_info.colormap[0] == nullptr ||
            m_info.colormap[1] == nullptr ||
            m_info.colormap[2] == nullptr) {
            jpeg_abort_decompress(&m_info);
            return 0;
        }
        // storing generated palette
        for (int i = 0; i < 256; i++) {
          dstPal[i << 2]       = m_info.colormap[0][i];
          dstPal[(i << 2) + 1] = m_info.colormap[1][i];
          dstPal[(i << 2) + 2] = m_info.colormap[2][i];
          dstPal[(i << 2) + 3] = 0;
        }
      } else {
        jpeg_abort_decompress(&m_info);
        return 0;
      }
    }

    // finished decompressing
    jpeg_finish_decompress(&m_info);

    return 1024 + len;
  }
  return 0;
}


bool Jpeg::updateInformation(uint8_t *buf, size_t bufSize) noexcept
{
  bool retVal = false;
  if (buf != nullptr && bufSize > 0) {
    jpeg_mem_src(&m_info, buf, bufSize);
    if (jpeg_read_header(&m_info, TRUE) != 0) {
      m_width = m_info.image_width;
      m_height = m_info.image_height;
      retVal = true;
    }
    jpeg_abort_decompress(&m_info);
  }
  return retVal;
}


void Jpeg::MyErrorExit(j_common_ptr cinfo)
{
  MyErrorMsgPtr myErr = (MyErrorMsgPtr)cinfo->err;
  (*cinfo->err->output_message)(cinfo);
  longjmp(myErr->setjmp_buffer, 1);
}


}   // namespace tc
