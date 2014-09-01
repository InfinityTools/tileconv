#########################
# Makefile for tileconv #
#########################

include config.mk

CXX         ?= g++
CXXFLAGS    = -c -Wall -O2 -std=c++11 -I$(ZLIB_INCLUDE) -I$(SQUISH_INCLUDE) -I$(PNGQUANT_INCLUDE)
LDFLAGS     = -L$(ZLIB_LIB) -L$(SQUISH_LIB) -L$(PNGQUANT_LIB)
SOURCES     = tileconv.cpp convert.cpp version.cpp graphics.cpp tilethreadpool.cpp tiledata.cpp compress.cpp colors.cpp fileio.cpp colorquant.cpp options.cpp
SYSLIBS     = -pthread
LIBS        =-lz -lsquish -limagequant $(SYSLIBS)
OBJECTS     = $(SOURCES:.cpp=.o)
EXECUTABLE  = tileconv
ifeq ($(OS),Windows_NT)
  RM        = del
  LDFLAGS   += -static
else
  RM        = rm
  ifeq ($(shell uname -s),Darwin)
    # A few hacks to enable proper threading support in Mac OS X
    CXXFLAGS += -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD
  else
    # Nothing to do for Linux (yet)
  endif
endif

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	$(RM) *.o
