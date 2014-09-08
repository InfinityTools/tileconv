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
#ifndef _OPTIONS_H_
#define _OPTIONS_H_
#include <string>
#include <vector>
#include <unordered_map>
#include "types.h"


/** Handles options parsing and storage. */
class Options
{
public:
  /** Attempts to determine the type of the given file. */
  static FileType GetFileType(const std::string &fileName) noexcept;

  /** Returns the given input filename with the new file extension as specified by type. */
  static std::string SetFileExt(const std::string &fileName, FileType type) noexcept;

  /** Returns the encoding type for the given numeric code. Returns Encoding::UNKNOWN on error. */
  static Encoding GetEncodingType(int code) noexcept;

  /** Returns whether the given code includes zlib compressed tiles. */
  static bool IsTileDeflated(int code) noexcept;

  /** Returns the numeric code of the given encoding type. Returns -1 on error. */
  static unsigned GetEncodingCode(Encoding type, bool deflate) noexcept;

  /** Returns a descriptive name of the given encoding type. */
  static const std::string& GetEncodingName(int code) noexcept;

public:
  Options() noexcept;
  ~Options() noexcept;

  /** Initialize options from the specified arguments. */
  bool init(int argc, char *argv[]) noexcept;

  /** Display a short syntax help. */
  void showHelp() noexcept;

  /** Methods for managing input files. */
  bool addInput(const std::string &inFile) noexcept;
  void removeInput(int idx) noexcept;
  void clearInput() noexcept { m_inFiles.clear(); }
  int getInputCount() const noexcept { return m_inFiles.size(); }
  const std::string& getInput(int idx) const noexcept;

  /** Define output file name (for single file conversion only). */
  bool setOutput(const std::string &outFile) noexcept;
  /** Call to activate auto-generation of output filename. */
  void resetOutput() noexcept;
  bool isOutput() const noexcept { return !m_outFile.empty(); }
  const std::string& getOutput() const noexcept { return m_outFile; }

  /** Cancel operation on error? Only effective when processing multiple files. */
  void setHaltOnError(bool b) noexcept { m_haltOnError = b; }
  bool isHaltOnError() const noexcept { return m_haltOnError; }

  /** Level of text output. 2=verbose, 1=summary only, 0=no output. */
  void setVerbosity(int level) noexcept;
  int getVerbosity() const noexcept { return m_verbosity; }
  /** Check for specific verbosity levels. */
  bool isSilent() const noexcept { return m_verbosity < 1; }
  bool isVerbose() const noexcept { return m_verbosity > 1; }

  /** Create MOSC files (MBC->MOS conversion only)? */
  void setMosc(bool b) noexcept { m_mosc = b; }
  bool isMosc() const noexcept { return m_mosc; }

  /** Color reduction quality in range 0..9. Calculate "r = 10 - quality" to get real value. */
  void setQuality(int v) noexcept;
  int getQuality() const noexcept { return m_quality; }

  /** Apply zlib compression to tiles? */
  void setDeflate(bool b) noexcept { m_deflate = b; }
  bool isDeflate() const noexcept { return m_deflate; }

  /** Number of threads to use for encoding/decoding. (0=autodetect) */
  void setThreads(int v) noexcept;
  int getThreads() const noexcept;

  /** Show statistics about the specified input file(s) only. */
  void setShowInfo(bool b) noexcept { m_showInfo = b; }
  bool isShowInfo() const noexcept { return m_showInfo; }

  /** Specify encoding type. */
  void setEncoding(Encoding type) noexcept { m_encoding = type; }
  Encoding getEncoding() const noexcept { return m_encoding; }

  /**
   * Returns a string containing a list of options in textual form.
   * \param complete If false, only options differing from the defaults are listed,
   *                 if true, all options are listed.
   * \return The list of options as text
   */
  std::string getOptionsSummary(bool complete) const noexcept;

private:
  static const int          MAX_NAME_LENGTH;    // max. filepath length
  static const int          MAX_THREADS;        // max. number of threads
  static const int          DEFLATE;            // !DEFLATE deflates

  // default values for options
  static const bool         DEF_HALT_ON_ERROR;
  static const bool         DEF_MOSC;
  static const bool         DEF_DEFLATE;
  static const bool         DEF_SHOWINFO;
  static const int          DEF_VERBOSITY;
  static const int          DEF_QUALITY;
  static const int          DEF_THREADS;
  static const Encoding     DEF_ENCODING;

  static const char         ParamNames[];

  bool                      m_haltOnError;  // cancel operation on error (when processing multiple files)
  bool                      m_mosc;         // create MOSC output
  bool                      m_deflate;      // apply zlib compression to TBC/MBC
  bool                      m_showInfo;
  int                       m_verbosity;    // verbosity level (2:verbose, 1:summary only, 0:no output)
  int                       m_quality;      // color reduction quality (0:fast, 9:slow)
  int                       m_threads;      // how many threads to use for encoding/decoding (0=auto)
  Encoding                  m_encoding;     // encoding type
  std::vector<std::string>  m_inFiles;
  std::string               m_outFile;
};



#endif		// _OPTIONS_H_
