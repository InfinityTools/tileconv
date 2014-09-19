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
#ifndef _TILETHREADPOOL_WIN32_H_
#define _TILETHREADPOOL_WIN32_H_

#ifdef USE_WINTHREADS
#include <windows.h>
#include "tilethreadpool_base.h"

namespace tc {

class TileThreadPoolWin32 : public TileThreadPool
{
public:
  TileThreadPoolWin32(unsigned threadNum, unsigned tileNum) noexcept;
  ~TileThreadPoolWin32() noexcept;

  /** See TileThreadPool::addTileData() */
  void addTileData(TileDataPtr tileData) noexcept;

  /** See TileThreadPool::getResult() */
  TileDataPtr getResult() noexcept;
  /** See TileThreadPool::peekResult() */
  const TileDataPtr peekResult() noexcept;
  /** See TileThreadPool::waitForResult() */
  void waitForResult() noexcept;

  /** See TileThreadPool::finished() */
  bool finished() noexcept;

protected:
  void threadActivated() noexcept;
  void threadDeactivated() noexcept;
  int getActiveThreads() noexcept { return m_activeThreads; }

private:
  // Executed by each thread. lpParam points to the current TileThreadPoolWin32 class instance.
  static DWORD WINAPI threadMain(LPVOID lpParam);

private:
  int                       m_activeThreads;
  HANDLE                    m_mainThread;
  HANDLE                    m_activeMutex;
  HANDLE                    m_tilesMutex;
  HANDLE                    m_resultsMutex;
//  std::vector<HANDLE>       m_threads;
  unsigned                  m_threadsNum;
  std::shared_ptr<HANDLE>   m_threads;
};

}   // namespace tc

#endif    // USE_WINTHREADS

#endif		// _TILETHREADPOOL_WIN32_H_
