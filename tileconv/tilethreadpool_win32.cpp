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
#ifdef USE_WINTHREADS
#include "graphics.h"
#include "tilethreadpool_win32.h"


unsigned getThreadPoolAutoThreads()
{
  SYSTEM_INFO sysinfo;
  ::GetSystemInfo(&sysinfo);
  return std::max(1u, (unsigned)sysinfo.dwNumberOfProcessors);
}


ThreadPoolPtr createThreadPool(Graphics &gfx, int threadNum, int tileNum)
{
  return ThreadPoolPtr(new TileThreadPoolWin32(gfx, threadNum, tileNum));
}


TileThreadPoolWin32::TileThreadPoolWin32(Graphics &gfx, unsigned threadNum, unsigned tileNum) noexcept
: TileThreadPool(gfx, tileNum)
, m_activeThreads(0)
, m_mainThread(::GetCurrentThread())
, m_activeMutex(::CreateMutex(NULL, FALSE, NULL))
, m_tilesMutex(::CreateMutex(NULL, FALSE, NULL))
, m_resultsMutex(::CreateMutex(NULL, FALSE, NULL))
, m_threadsNum()
, m_threads(nullptr)
{
  m_threadsNum = std::max(1u, std::min(MAX_THREADS, threadNum));
  m_threads.reset(new HANDLE[m_threadsNum], std::default_delete<HANDLE[]>());
  for (unsigned i = 0; i < threadNum; i++) {
    m_threads.get()[i] = ::CreateThread(NULL, 0, TileThreadPoolWin32::threadMain, this, 0, NULL);
  }
}


TileThreadPoolWin32::~TileThreadPoolWin32() noexcept
{
  setTerminate(true);
  ::WaitForMultipleObjects(m_threadsNum, m_threads.get(), TRUE, INFINITE);
  for (unsigned i = 0; i < m_threadsNum; i++) {
    ::CloseHandle(m_threads.get()[i]);
  }
  ::CloseHandle(m_activeMutex);
  ::CloseHandle(m_tilesMutex);
  ::CloseHandle(m_resultsMutex);
}


void TileThreadPoolWin32::addTileData(TileDataPtr tileData) noexcept
{
  while (!canAddTileData()) {
    ::Sleep(50);
  }

  ::WaitForSingleObject(m_tilesMutex, INFINITE);
  getTileQueue().emplace(tileData);
  ::ReleaseMutex(m_tilesMutex);
}


TileDataPtr TileThreadPoolWin32::getResult() noexcept
{
  ::WaitForSingleObject(m_resultsMutex, INFINITE);
  if (!getResultQueue().empty()) {
    TileDataPtr retVal = getResultQueue().top();
    getResultQueue().pop();
    ::ReleaseMutex(m_resultsMutex);
    return retVal;
  } else {
    ::ReleaseMutex(m_resultsMutex);
    return TileDataPtr(nullptr);
  }
}


const TileDataPtr TileThreadPoolWin32::peekResult() noexcept
{
  ::WaitForSingleObject(m_resultsMutex, INFINITE);
  if (!getResultQueue().empty()) {
    TileDataPtr retVal = getResultQueue().top();
    ::ReleaseMutex(m_resultsMutex);
    return retVal;
  } else {
    ::ReleaseMutex(m_resultsMutex);
    return TileDataPtr(nullptr);
  }
}


void TileThreadPoolWin32::waitForResult() noexcept
{
  while (!hasResult() && !finished()) {
    ::Sleep(50);
  }
}


bool TileThreadPoolWin32::finished() noexcept
{
  HANDLE locks[] = { m_tilesMutex, m_activeMutex, m_resultsMutex };
  ::WaitForMultipleObjects(3, locks, TRUE, INFINITE);
  bool retVal = (getTileQueue().empty() && getActiveThreads() == 0 && getResultQueue().empty());
  ::ReleaseMutex(m_tilesMutex);
  ::ReleaseMutex(m_activeMutex);
  ::ReleaseMutex(m_resultsMutex);
  return retVal;
}


DWORD WINAPI TileThreadPoolWin32::threadMain(LPVOID lpParam)
{
  TileThreadPoolWin32 *instance = (TileThreadPoolWin32*)lpParam;
  if (instance != nullptr) {
    while (!instance->terminate()) {
      ::WaitForSingleObject(instance->m_tilesMutex, INFINITE);
      if (!instance->getTileQueue().empty()) {
        instance->threadActivated();

        TileDataPtr tileData = instance->getTileQueue().front();
        instance->getTileQueue().pop();
        ::ReleaseMutex(instance->m_tilesMutex);

        tileData = instance->getGraphics().processTile(tileData);

        // storing results
        ::WaitForSingleObject(instance->m_resultsMutex, INFINITE);
        instance->getResultQueue().push(tileData);
        ::ReleaseMutex(instance->m_resultsMutex);

        instance->threadDeactivated();
      } else {
        ::ReleaseMutex(instance->m_tilesMutex);
        ::Sleep(50);
      }
    }
  }
  return 0;
}


void TileThreadPoolWin32::threadActivated() noexcept
{
  ::WaitForSingleObject(m_activeMutex, INFINITE);
  m_activeThreads++;
  ::ReleaseMutex(m_activeMutex);
}


void TileThreadPoolWin32::threadDeactivated() noexcept
{
  ::WaitForSingleObject(m_activeMutex, INFINITE);
  m_activeThreads--;
  ::ReleaseMutex(m_activeMutex);
}


#endif    // USE_WINTHREADS
