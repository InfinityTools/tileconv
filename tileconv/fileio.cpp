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
#include "fileio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace tc {

bool File::IsDirectory(const char *fileName) noexcept
{
  if (fileName != nullptr) {
    struct stat s;
    if (stat(fileName, &s) != -1) {
      return S_ISDIR(s.st_mode);
    }
  }
  return false;
}


long File::GetFileSize(const char *fileName) noexcept
{
  if (fileName != nullptr) {
    struct stat s;
    if (stat(fileName, &s) != -1) {
      return s.st_size;
    }
  }
  return -1L;
}


bool File::RemoveFile(const char *fileName) noexcept
{
  if (fileName != nullptr) {
    return (std::remove(fileName) == 0);
  }
  return false;
}


bool File::RenameFile(const char *oldFileName, const char *newFileName) noexcept
{
  if (oldFileName != nullptr && *oldFileName != 0 &&
      newFileName != nullptr && *newFileName != 0) {
    return (std::rename(oldFileName, newFileName) == 0);
  }
  return false;
}


File::File(const char *fileName, const char *mode) noexcept
: m_file(0)
, m_fileName()
, m_mode()
, m_buffer(0)
, m_deleteOnClose(false)
{
  m_file = std::fopen(fileName, mode);
  if (m_file != nullptr) {
    m_fileName.assign(fileName);
    m_mode.assign(mode);
  }
}

File::File(const char *fileName, const char *mode, int bufferSize) noexcept
: m_file(0)
, m_fileName()
, m_mode()
, m_buffer(0)
{
  m_file = std::fopen(fileName, mode);
  if (m_file != nullptr) {
    m_fileName.assign(fileName);
    m_mode.assign(mode);
    if (bufferSize < 0) bufferSize = 0;
    if (bufferSize > 0) m_buffer = new char[bufferSize];
    std::setvbuf(m_file, m_buffer, (bufferSize > 0) ? _IOFBF : _IONBF, bufferSize);
  }
}

File::~File() noexcept
{
  if (m_file != nullptr) {
    flush();
    std::fclose(m_file);
    m_file = nullptr;
  }
  if (m_buffer != nullptr) {
    delete[] m_buffer;
    m_buffer = nullptr;
  }
  if (isDeleteOnClose() && !m_fileName.empty()) {
    std::remove(m_fileName.c_str());
  }
}

bool File::reopen(const char *fileName, const char *mode) noexcept
{
  if (m_file) {
    m_file = std::freopen(fileName, mode, m_file);
    if (m_file) {
      m_fileName.assign(fileName);
      m_mode.assign(mode);
      return true;
    } else {
      m_fileName.clear();
      m_mode.clear();
    }
  }
  return false;
}

bool File::flush() noexcept
{
  if (m_file) {
    return (std::fflush(m_file) != EOF);
  }
  return false;
}

std::size_t File::read(void *buffer, std::size_t size, std::size_t count) noexcept
{
  if (m_file) {
    return std::fread(buffer, size, count, m_file);
  }
  return 0;
}

std::size_t File::write(const void *buffer, std::size_t size, std::size_t count) noexcept
{
  if (m_file) {
    return std::fwrite(buffer, size, count, m_file);
  }
  return 0;
}

int File::getc() noexcept
{
  if (m_file) {
    return std::fgetc(m_file);
  }
  return EOF;
}

char* File::gets(char *str, int count) noexcept
{
  if (m_file) {
    return std::fgets(str, count, m_file);
  }
  return NULL;
}

int File::putc(int ch) noexcept
{
  if (m_file) {
    return std::fputc(ch, m_file);
  }
  return EOF;
}

int File::puts(const char *str) noexcept
{
  if (m_file) {
    return std::fputs(str, m_file);
  }
  return EOF;
}

int File::ungetc(int ch) noexcept
{
  if (m_file) {
    return std::ungetc(ch, m_file);
  }
  return EOF;
}

long File::tell() noexcept
{
  if (m_file) {
    return std::ftell(m_file);
  }
  return -1L;
}

bool File::getpos(std::fpos_t *pos) noexcept
{
  if (m_file) {
    return (std::fgetpos(m_file, pos) == 0);
  }
  return false;
}

bool File::seek(long offset, int origin) noexcept
{
  if (m_file) {
    return (std::fseek(m_file, offset, origin) == 0);
  }
  return false;
}

bool File::setpos(const std::fpos_t *pos) noexcept
{
  if (m_file) {
    return (std::fsetpos(m_file, pos) == 0);
  }
  return false;
}

void File::rewind() noexcept
{
  if (m_file) {
    std::rewind(m_file);
  }
}

void File::clearerr() noexcept
{
  if (m_file) {
    std::clearerr(m_file);
  }
}

bool File::eof() noexcept
{
  if (m_file) {
    return (std::feof(m_file) != 0);
  }
  return true;
}

bool File::error() noexcept
{
  if (m_file) {
    return (std::ferror(m_file) != 0);
  }
  return true;
}

void File::perror(const char *s) noexcept
{
  std::perror(s);
}


long File::getsize() noexcept
{
  if (m_file) {
    long curPos = tell();
    if (curPos >= 0 && seek(0, SEEK_END)) {
      long size = tell();
      seek(curPos, SEEK_SET);
      return size;
    }
  }
  return -1L;
}


bool File::isReadEnabled() const noexcept
{
  for (auto iter = m_mode.cbegin(); iter != m_mode.cend(); ++iter) {
    switch (*iter) {
      case 'r':
      case '+':
        return true;
      default:
        break;
    }
  }
  return false;
}


bool File::isWriteEnabled() const noexcept
{
  for (auto iter = m_mode.cbegin(); iter != m_mode.cend(); ++iter) {
    switch (*iter) {
      case 'w':
      case 'a':
      case '+':
        return true;
      default:
        break;
    }
  }
  return false;
}

}   // namespace tc
