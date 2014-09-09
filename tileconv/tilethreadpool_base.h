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
#ifndef _TILETHREADPOOL_BASE_H_
#define _TILETHREADPOOL_BASE_H_
#include <queue>
#include "tiledata.h"
#include "graphics.h"

class TileThreadPool;

typedef std::shared_ptr<TileThreadPool> ThreadPoolPtr;


/**
 * Detected number of separate threads on the current CPU.
 * (Defined together with specialized thread pool classes.)
 */
unsigned getThreadPoolAutoThreads();

/**
 * Dynamically create and return a new thread pool instance encapsulated in a smart pointer.
 * (Defined together with specialized thread pool classes.)
 */
ThreadPoolPtr createThreadPool(Graphics &gfx, int threadNum, int tileNum);


class TileThreadPool
{
public:
  /** Max. number of supported threads. */
  static const unsigned MAX_THREADS;

  /** Max. number of allowed tile data blocks to add for processing. */
  static const unsigned MAX_TILES;

//  static unsigned GetMaxThreads() noexcept;

  /** Detected number of separate threads on the current CPU. */
//  static unsigned GetAutoThreads() noexcept;

//  static unsigned GetMaxTiles() noexcept;

public:
  virtual ~TileThreadPool() noexcept {}

  /** Get/set max. number of tile data blocks to add. */
  unsigned getMaxTiles() const noexcept { return m_maxTiles; }
  void setMaxTiles(unsigned maxTiles) noexcept;

  /** Add tile data to input queue. Blocks execution as long as the input queue is full. */
  virtual void addTileData(TileDataPtr tileData) noexcept = 0;
  /** Returns whether you can still add new tile data blocks to the input queue. */
  bool canAddTileData() const noexcept { return (m_tiles.size() < m_maxTiles); }

  /** Returns whether results are waiting to be returned. */
  bool hasResult() const noexcept { return !m_results.empty(); }
  /** Returns the next result if available or null pointer otherwise. */
  virtual TileDataPtr getResult() noexcept = 0;
  /** Provides read-only access to the next result or a null pointer otherwise. */
  virtual const TileDataPtr peekResult() noexcept = 0;
  /** Waits until a result is ready or the threadpool is idle. */
  virtual void waitForResult() noexcept = 0;

  /**
   * Returns if all data blocks in the input queue have been processed and
   * pushed onto the result queue.
   */
  virtual bool finished() noexcept = 0;


protected:
  typedef std::queue<TileDataPtr> TileQueue;
  typedef std::priority_queue<TileDataPtr, std::vector<TileDataPtr>, std::greater<TileDataPtr>> ResultQueue;

  TileThreadPool(Graphics &gfx, unsigned tileNum) noexcept;

  // Called whenever a thread is about to execute another encoding/decoding function
  virtual void threadActivated() noexcept = 0;
  // Called whenever a thread has finished the execution of an encoding/decoding function
  virtual void threadDeactivated() noexcept = 0;
  // Returns the number of active threads
  virtual int getActiveThreads() noexcept = 0;

  // Access to associated Graphics instance
  Graphics& getGraphics() noexcept { return m_gfx; }
  const Graphics& getGraphics() const noexcept { return m_gfx; }

  // Access to input queue
  TileQueue& getTileQueue() noexcept { return m_tiles; }
  const TileQueue& getTileQueue() const noexcept { return m_tiles; }

  // Access to result queue
  ResultQueue& getResultQueue() noexcept { return m_results; }
  const ResultQueue& getResultQueue() const noexcept { return m_results; }

  // Queried by each thread function
  bool terminate() const noexcept { return m_terminate; }
  // Called by the destructor to signal all threads to finish
  void setTerminate(bool b) noexcept { m_terminate = b; }

private:
  Graphics      &m_gfx;
  bool          m_terminate;
  unsigned      m_maxTiles;
  TileQueue     m_tiles;
  ResultQueue   m_results;
};


#endif		// _TILETHREADPOOL_BASE_H_
