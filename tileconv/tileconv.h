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
#ifndef _TILECONV_H_
#define _TILECONV_H_
#include "types.h"
#include "options.h"

namespace tc {

/** High level class for converting TIS<->TBC and MOS<->MBC. */
class TileConv
{
public:
  // Construct an uninitialized converter object.
  TileConv() noexcept;
  // Construct a converter object and initialize it with the specified arguments.
  TileConv(int argc, char *argv[]) noexcept;
  ~TileConv() noexcept;

  // Initialize the converter object with the specified arguments
  bool init(int argc, char *argv[]) noexcept;

  // Initiate conversion process
  bool execute() noexcept;

  const Options& getOptions() const noexcept { return m_options; }

private:
  // Display information about the specified filename
  bool showInfo(const std::string &fileName) noexcept;

  // Returns whether arguments have been initialized successfully.
  bool isInitialized() const noexcept { return m_initialized; }

private:
  Options   m_options;
  bool      m_initialized;
};

}   // namespace tc

#endif		// _TILECONV_H_
