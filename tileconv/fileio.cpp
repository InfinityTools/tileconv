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
#ifdef _WIN32
# include <windows.h>
#else
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
#endif

namespace tc {

#ifdef _WIN32
const char File::PATH_SEPARATOR = '\\';
#else
const char File::PATH_SEPARATOR = '/';
#endif


std::string File::ExtractFilePath(const std::string &fileName) noexcept
{
  std::string retVal;
  if (!fileName.empty()) {
    if (IsPathSeparator(fileName.at(fileName.size() - 1))) {
      retVal = fileName;
    } else {
      std::string::size_type pos = fileName.find_last_of(PATH_SEPARATOR);
      if (pos == std::string::npos) {
        pos = fileName.find_last_of('/');
      }
      if (pos != std::string::npos) {
        retVal = fileName.substr(0, pos);
      }
    }
  }
  return retVal;
}

std::string File::ExtractFileName(const std::string &fileName) noexcept
{
  std::string retVal;
  if (!fileName.empty() && !IsPathSeparator(fileName.at(fileName.size() - 1))) {
    std::string::size_type pos = fileName.find_last_of(PATH_SEPARATOR);
    if (pos == std::string::npos) {
      pos = fileName.find_last_of('/');
    }
    if (pos != std::string::npos) {
      retVal = fileName.substr(pos+1);
    } else {
      retVal = fileName;
    }
  }
  return retVal;
}

std::string File::ExtractFileBase(const std::string &fileName) noexcept
{
  std::string retVal = ExtractFileName(fileName);
  if (!retVal.empty()) {
    std::string::size_type pos = retVal.find_last_of('.');
    if (pos != std::string::npos) {
      retVal = retVal.substr(0, pos);
    }
  }
  return retVal;
}

std::string File::ExtractFileExt(const std::string &fileName) noexcept
{
  std::string retVal = ExtractFileName(fileName);
  std::string::size_type pos = retVal.find_last_of('.');
  if (pos != std::string::npos) {
    retVal = retVal.substr(pos);
  } else {
    retVal.clear();
  }
  return retVal;
}

std::string File::CreateFileName(const std::string &path, const std::string &file) noexcept
{
  std::string retVal;
  if (!path.empty()) {
    retVal += path;
    if (!IsPathSeparator(path.at(path.size() - 1))) {
      retVal += PATH_SEPARATOR;
    }
  }
  if (!file.empty()) {
    retVal += file;
  }
  return retVal;
}

std::string File::ChangeFileExt(const std::string &fileName, const std::string &fileExt) noexcept
{
  std::string retVal;
  if (!fileName.empty()) {
    std::string path = ExtractFilePath(fileName);
    std::string name = ExtractFileBase(fileName);
    if (!name.empty()) {
      if (!fileExt.empty() && fileExt.at(0) != '.') {
        name += '.';
      }
      name += fileExt;
      retVal = CreateFileName(path, name);
    } else {
      retVal = fileName;
    }
  }
  return retVal;
}

bool File::IsPathSeparator(char ch) noexcept
{
  // Note: '/' is always valid
  return (ch == PATH_SEPARATOR || ch == '/');
}

bool File::IsDirectory(const std::string &fileName) noexcept
{
  bool retVal = false;
  if (!fileName.empty()) {
#ifdef _WIN32
    DWORD flags = ::GetFileAttributes(fileName.c_str());
    if (flags != INVALID_FILE_ATTRIBUTES) {
      retVal = (flags & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
#else
    struct stat s;
    if (::stat(fileName.c_str(), &s) != -1) {
      return S_ISDIR(s.st_mode);
    }
#endif
  }
  return retVal;
}

bool File::Exists(const std::string &path) noexcept
{
  bool retVal = false;
  if (!path.empty()) {
#ifdef _WIN32
    retVal = (::GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES);
#else
    struct stat s;
    retVal = (::stat(path.c_str(), &s) == 0);
#endif
  }
  return retVal;
}

long File::GetFileSize(const std::string &fileName) noexcept
{
  if (!fileName.empty()) {
#ifdef _WIN32
    HANDLE h = ::CreateFile(fileName.c_str(), 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                            0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if (h != INVALID_HANDLE_VALUE) {
      DWORD size = ::GetFileSize(h, NULL);
      ::CloseHandle(h);
      if (size != INVALID_FILE_SIZE) {
        return size;
      }
    }
#else
    struct stat s;
    if (::stat(fileName.c_str(), &s) != -1) {
      return s.st_size;
    }
#endif
  }
  return -1L;
}

bool File::RemoveFile(const std::string &fileName) noexcept
{
  if (!fileName.empty()) {
    return (std::remove(fileName.c_str()) == 0);
  }
  return false;
}

bool File::RenameFile(const std::string &oldFileName, const std::string &newFileName) noexcept
{
  if (!oldFileName.empty() && !newFileName.empty()) {
    return (std::rename(oldFileName.c_str(), newFileName.c_str()) == 0);
  }
  return false;
}

bool File::IsEqual(const std::string &path1, const std::string &path2) noexcept
{
  bool retVal = false;

  if (!path1.empty() && !path2.empty()) {
#ifdef _WIN32
    HANDLE h2 = ::CreateFile(path2.c_str(), 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    HANDLE h1 = ::CreateFile(path1.c_str(), 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if (h1 == INVALID_HANDLE_VALUE || h2 == INVALID_HANDLE_VALUE) {
      if (h2 != INVALID_HANDLE_VALUE) {
        ::CloseHandle(h2);
      }
      if (h1 != INVALID_HANDLE_VALUE) {
        ::CloseHandle(h1);
      }
      return retVal;
    }

    BY_HANDLE_FILE_INFORMATION info1, info2;
    bool b1 = ::GetFileInformationByHandle(h1, &info1);
    bool b2 = ::GetFileInformationByHandle(h2, &info2);
    if (!b1 || !b2) {
      ::CloseHandle(h2);
      ::CloseHandle(h1);
      return retVal;
    }

    retVal = (info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber) &&
             (info1.nFileIndexHigh == info2.nFileIndexHigh) &&
             (info1.nFileIndexLow == info2.nFileIndexLow) &&
             (info1.nFileSizeHigh == info2.nFileSizeHigh) &&
             (info1.nFileSizeLow == info2.nFileSizeLow) &&
             (info1.ftLastWriteTime.dwLowDateTime == info2.ftLastWriteTime.dwLowDateTime) &&
             (info1.ftLastWriteTime.dwHighDateTime == info2.ftLastWriteTime.dwHighDateTime);

      ::CloseHandle(h2);
      ::CloseHandle(h1);

#else
      struct stat s1, s2;
      int e2 = ::stat(path2.c_str(), &s2);
      int e1 = ::stat(path1.c_str(), &s1);
      if (e1 == 0 && e2 == 0) {
        retVal = (s1.st_dev == s2.st_dev) &&
                 (s1.st_ino == s2.st_ino) &&
                 (s1.st_size == s2.st_size) &&
                 (s1.st_mtime == s2.st_mtime);
      }

#endif
  }
  return retVal;
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
, m_deleteOnClose(false)
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
