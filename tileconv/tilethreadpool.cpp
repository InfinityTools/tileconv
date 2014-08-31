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
#include "graphics.h"
#include "tilethreadpool.h"


const unsigned TileThreadPool::MAX_THREADS = 256u;
const unsigned TileThreadPool::AUTO_THREADS = std::max(1u, std::min(MAX_THREADS, std::thread::hardware_concurrency()));
const unsigned TileThreadPool::MAX_TILES = std::numeric_limits<int>::max();


TileThreadPool::TileThreadPool(Graphics &gfx, unsigned threadNum, unsigned tileNum) noexcept
: m_gfx(gfx)
, m_terminate(false)
, m_maxTiles(MAX_TILES)
, m_activeThreads(0)
, m_activeMutex()
, m_tilesMutex()
, m_resultsMutex()
, m_tiles()
, m_threads()
, m_results()
{
  setMaxTiles(tileNum);
  threadNum = std::max(1u, std::min(MAX_THREADS, threadNum));
  for (unsigned i = 0; i < threadNum; i++) {
    m_threads.emplace_back(std::thread(&TileThreadPool::threadMain, this));
  }
}


TileThreadPool::~TileThreadPool() noexcept
{
  setTerminate(true);
  for (auto iter = m_threads.begin(); iter != m_threads.end(); ++iter) {
    iter->join();
  }
}




bool TileThreadPool::addTileData(TileDataPtr tileData) noexcept
{
  std::lock_guard<std::mutex> lock(m_tilesMutex);
  if (m_tiles.size() < getMaxTiles()) {
    m_tiles.push(tileData);
    return true;
  }
  return false;
}


TileDataPtr TileThreadPool::getResult() noexcept
{
  std::lock_guard<std::mutex> lock(m_resultsMutex);
  if (!m_results.empty()) {
    TileDataPtr retVal = m_results.top();
    m_results.pop();
    return retVal;
  } else {
    return TileDataPtr(nullptr);
  }
}


const TileDataPtr TileThreadPool::peekResult() noexcept
{
  std::lock_guard<std::mutex> lock(m_resultsMutex);
  if (!m_results.empty()) {
    return m_results.top();
  } else {
    return TileDataPtr(nullptr);
  }
}


bool TileThreadPool::finished() noexcept
{
  std::unique_lock<std::mutex> lock1(m_tilesMutex, std::defer_lock);
  std::unique_lock<std::mutex> lock2(m_activeMutex, std::defer_lock);
  std::unique_lock<std::mutex> lock3(m_resultsMutex, std::defer_lock);
  std::lock(lock1, lock2, lock3);
  bool retVal = (m_tiles.empty() && m_activeThreads == 0 && m_results.empty());
  lock1.unlock(); lock2.unlock(); lock3.unlock();
  return retVal;
}


void TileThreadPool::threadMain() noexcept
{
  std::unique_lock<std::mutex> lockTiles(m_tilesMutex, std::defer_lock);
  std::unique_lock<std::mutex> lockResults(m_resultsMutex, std::defer_lock);
  while (!terminate()) {
    lockTiles.lock();
    if (!m_tiles.empty()) {
      threadActivated();

      TileDataPtr tileData = m_tiles.front();
      m_tiles.pop();
      lockTiles.unlock();
      if (tileData != nullptr) {
        // executing encoding/decoding methods
        if (tileData->isEncoding) {
          try {
            tileData = m_gfx.encodeTile(tileData);
          } catch (...) {
            tileData = TileDataPtr(nullptr);
          }
        } else {
          try {
            tileData = m_gfx.decodeTile(tileData);
          } catch (...) {
            tileData = TileDataPtr(nullptr);
          }
        }
      }

      // storing results
      lockResults.lock();
      m_results.push(tileData);
      lockResults.unlock();

      threadDeactivated();
    } else {
      lockTiles.unlock();
      std::this_thread::yield();
    }
  }
}


void TileThreadPool::threadActivated() noexcept
{
  std::lock_guard<std::mutex> lock(m_activeMutex);
  m_activeThreads++;
}


void TileThreadPool::threadDeactivated() noexcept
{
  std::lock_guard<std::mutex> lock(m_activeMutex);
  m_activeThreads--;
}
