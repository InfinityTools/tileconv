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
#ifndef _TILETHREADPOOL_H_
#define _TILETHREADPOOL_H_
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include "tiledata.h"

class Graphics;

/** Provides threading capabilities specialized for encoding or decoding tile data. */
class TileThreadPool
{
public:
  /** Max. number of supported threads. */
  static const unsigned MAX_THREADS;
  /** Detected number of separate threads on the current CPU. */
  static const unsigned AUTO_THREADS;
  /** Max. number of allowed tile data blocks to add for processing. */
  static const unsigned MAX_TILES;

public:
  TileThreadPool(Graphics &gfx, unsigned threadNum = AUTO_THREADS, unsigned tileNum = MAX_TILES) noexcept;
  ~TileThreadPool() noexcept;

  /** Get/set max. number of tile data blocks to add. */
  unsigned getMaxTiles() const noexcept { return m_maxTiles; }
  void setMaxTiles(unsigned maxTiles) noexcept { m_maxTiles = std::max(1u, std::min(MAX_TILES, maxTiles)); }

  /** Add tile data to queue. Returns true if successfully added, false otherwise. */
  bool addTileData(TileDataPtr tileData) noexcept;
  /** Returns whether you can still add new tile data blocks to the waiting queue. */
  bool canAddTileData() const noexcept { return (m_tiles.size() < m_maxTiles); }

  /** Returns whether results are waiting to be returned. */
  bool hasResult() const noexcept { return !m_results.empty(); }
  /** Returns the next result if available or null pointer otherwise. */
  TileDataPtr getResult() noexcept;
  /** Provides read-only access to the next result or a null pointer otherwise. */
  const TileDataPtr peekResult() noexcept;

  /** Returns if all data blocks in the queue have been processed and pushed onto the result queue. */
  bool finished() noexcept;

private:
  // Executed by each thread.
  void threadMain() noexcept;

  // Called whenever a thread is about to execute another encoding/decoding function
  void threadActivated() noexcept;
  // Called whenever a thread has finished the execution of an encoding/decoding function
  void threadDeactivated() noexcept;

  // Queried by each thread function
  bool terminate() const noexcept { return m_terminate; }
  // Called by the destructor to signal all threads to finish
  void setTerminate(bool b) noexcept { m_terminate = b; }

private:
  Graphics                  &m_gfx;
  bool                      m_terminate;
  unsigned                  m_maxTiles;
  int                       m_activeThreads;
  std::mutex                m_activeMutex;
  std::mutex                m_tilesMutex;
  std::mutex                m_resultsMutex;
  std::queue<TileDataPtr>   m_tiles;
  std::vector<std::thread>  m_threads;
  std::priority_queue<TileDataPtr, std::vector<TileDataPtr>, std::greater<TileDataPtr>> m_results;
};


#endif		// _TILETHREADPOOL_H_
