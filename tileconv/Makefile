#########################
# Makefile for tileconv #
#########################

include config.mk

CXX           ?= g++
CXXFLAGS      = -c -Wall -O2 -std=c++11 -msse -mfpmath=sse -I$(ZLIB_INCLUDE) -I$(PNGQUANT_INCLUDE) -I$(SQUISH_INCLUDE)
LDFLAGS       = -L$(ZLIB_LIB) -L$(PNGQUANT_LIB) -L$(SQUISH_LIB)
LIBS          = -lz -limagequant -lsquish
EXECUTABLE    = tileconv

ifeq ($(OS),Windows_NT)
  EXT         = .exe
  RM          = del
  LDFLAGS     += -static
  ifeq ($(USE_WINTHREADS),1)
    CXXFLAGS    += -mwindows -DUSE_WINTHREADS=1
  else
    LIBS        += -pthread
  endif
else
  RM          = rm
  LIBS        += -pthread
  ifeq ($(shell uname -s),Darwin)
    # A few hacks to enable proper threading support
    CXXFLAGS    += -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD
  else
    # Nothing to do for Linux (yet)
  endif
endif

SOURCES = \
  tileconv.cpp \
  convert.cpp \
  version.cpp \
  graphics.cpp \
  tilethreadpool_base.cpp \
  tilethreadpool_posix.cpp \
  tilethreadpool_win32.cpp \
  tiledata.cpp \
  compress.cpp \
  colors.cpp \
  fileio.cpp \
  colorquant.cpp \
  options.cpp \
  dxtbase.cpp \
  dxtsquish.cpp

OBJECTS = $(SOURCES:.cpp=.o)


all: $(SOURCES) $(EXECUTABLE)

install: $(EXECUTABLE)$(EXT)
ifeq ($(OS),Windows_NT)
	@echo Target not supported.
else
	install $(EXECUTABLE) $(INSTALL_DIR)/$(EXECUTABLE)
endif

uninstall:
ifeq ($(OS),Windows_NT)
	@echo Target not supported.
else
	$(RM) $(INSTALL_DIR)/$(EXECUTABLE)
endif

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

clean:
	$(RM) $(OBJECTS)
#	$(RM) *.o
