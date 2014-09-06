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
#include <algorithm>
#include "tilethreadpool_base.h"


const unsigned TileThreadPool::MAX_THREADS  = 256u;
const unsigned TileThreadPool::MAX_TILES    = std::numeric_limits<int>::max();


TileThreadPool::TileThreadPool(Graphics &gfx, unsigned tileNum) noexcept
: m_gfx(gfx)
, m_terminate(false)
, m_maxTiles(MAX_TILES)
//, m_activeThreads(0)
, m_tiles()
, m_results()
{
  setMaxTiles(tileNum);
}


TileThreadPool::~TileThreadPool() noexcept
{
}


void TileThreadPool::setMaxTiles(unsigned maxTiles) noexcept
{
  m_maxTiles = std::max(1u, std::min(MAX_TILES, maxTiles));
}


Graphics& TileThreadPool::getGraphics() noexcept { return m_gfx; }


const Graphics& TileThreadPool::getGraphics() const noexcept { return m_gfx; }
