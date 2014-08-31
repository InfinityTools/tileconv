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
#include <cstdint>
#include <algorithm>
#include "colorquant.h"

ColorQuant::ColorQuant() noexcept
: m_dithering(false)
, m_lastTransparent(false)
, m_maxColors(256)
, m_qualityMin(0)
, m_qualityMax(100)
, m_speed(3)
, m_minOpacity(255)
, m_posterization(0)
, m_source(nullptr)
, m_width(0)
, m_height(0)
, m_gamma(0.0)
, m_target(nullptr)
, m_targetSize(0)
, m_palette(nullptr)
, m_paletteSize(0)
, m_liqAttr(nullptr)
, m_liqImage(nullptr)
, m_liqResult(nullptr)
{
}


ColorQuant::~ColorQuant() noexcept
{
  freeMemory();
}


bool ColorQuant::setSource(void *bitmap, int width, int height, double gamma) noexcept
{
  if (bitmap && width > 0 && height > 0 && gamma >= 0.0 && gamma <= 1.0) {
    m_source = bitmap;
    m_width = width;
    m_height = height;
    m_gamma = gamma;
    return true;
  }
  return false;
}


bool ColorQuant::setTarget(void *buffer, int size) noexcept
{
  if (buffer && size > 0) {
    int minSize = m_width*m_height;
    if (minSize > 0 && size < minSize) {
      return false;
    }
    m_target = buffer;
    m_targetSize = size;
    return true;
  }
  return false;
}


bool ColorQuant::setPalette(void *palette, int size) noexcept
{
  if (palette && size >= m_maxColors*4) {
    m_palette = palette;
    m_paletteSize = size;
    return true;
  }
  return false;
}


bool ColorQuant::quantize() noexcept
{
  freeMemory();

  // initializing attributes
  if ((m_liqAttr = liq_attr_create()) == nullptr) {
    return false;
  }
  if (m_maxColors < 256) {
    liq_set_max_colors(m_liqAttr, m_maxColors);
  }
  if (m_qualityMin != 0 && m_qualityMax != 100) {
    liq_set_quality(m_liqAttr, m_qualityMin, m_qualityMax);
  }
  if (m_speed != 3) {
    liq_set_speed(m_liqAttr, m_speed);
  }
  if (m_minOpacity != 255) {
    liq_set_min_opacity(m_liqAttr, m_minOpacity);
  }
  liq_set_min_posterization(m_liqAttr, m_posterization);
  if (m_lastTransparent) {
    liq_set_last_index_transparent(m_liqAttr, 1);
  }

  // quantizing image
  if ((m_liqImage = liq_image_create_rgba(m_liqAttr, m_source, m_width, m_height, m_gamma)) == nullptr) {
    return false;
  }
  if ((m_liqResult = liq_quantize_image(m_liqAttr, m_liqImage)) == nullptr) {
    return false;
  }

  if (LIQ_OK != liq_write_remapped_image(m_liqResult, m_liqImage, m_target, m_targetSize)) {
    return false;
  }

  // storing palette
  const liq_palette *pal = liq_get_palette(m_liqResult);
  unsigned numCols = std::min(pal->count, m_paletteSize >> 2);
  uint8_t *dstPal = (uint8_t*)m_palette;
  for (unsigned i = 0; i < numCols; i++, dstPal += 4) {
    liq_color px = pal->entries[i];
    if (px.a < 255) {
      dstPal[0] = dstPal[2] = dstPal[3] = 0;
      dstPal[1] = 255;
    } else {
      dstPal[0] = px.r;
      dstPal[1] = px.g;
      dstPal[2] = px.b;
      dstPal[3] = 0;
    }
  }

  return true;
}


void ColorQuant::setMaxColors(int colors) noexcept
{
  m_maxColors = std::max(2, std::min(256, colors));
}

void ColorQuant::setQuality(int min, int max) noexcept
{
  min = std::max(0, std::min(100, min));
  max = std::max(0, std::min(100, max));
  if (min > max) std::swap(min, max);
  m_qualityMin = min;
  m_qualityMax = max;
}

void ColorQuant::setSpeed(int speed) noexcept
{
  m_speed = std::max(1, std::min(10, speed));
}

void ColorQuant::setMinOpacity(int min) noexcept
{
  m_minOpacity = std::max(0, std::min(255, min));
}

void ColorQuant::setPosterization(int bits) noexcept
{
  m_posterization = std::max(0, std::min(4, bits));
}

double ColorQuant::getQuantizationError() noexcept
{
  if (m_liqResult) {
    return liq_get_quantization_error(m_liqResult);
  }
  return -1.0;
}

double ColorQuant::getQuantizationQuality() noexcept
{
  if (m_liqResult) {
    return liq_get_quantization_quality(m_liqResult);
  }
  return -1.0;
}


void ColorQuant::freeMemory() noexcept
{
  if (m_liqAttr) {
    liq_attr_destroy(m_liqAttr);
    m_liqAttr = nullptr;
  }
  if (m_liqImage) {
    liq_image_destroy(m_liqImage);
    m_liqImage = nullptr;
  }
  if (m_liqResult) {
    liq_result_destroy(m_liqResult);
    m_liqResult = nullptr;
  }
}
