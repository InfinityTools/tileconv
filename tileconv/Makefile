#########################
# Makefile for tileconv #
#########################

include config.mk

CXX         ?= g++
CXXFLAGS    = -c -Wall -O2 -std=c++11 -I$(ZLIB_INCLUDE) -I$(SQUISH_INCLUDE) -I$(PNGQUANT_INCLUDE)
LDFLAGS     = -L$(ZLIB_LIB) -L$(SQUISH_LIB) -L$(PNGQUANT_LIB)
SOURCES     = tileconv.cpp convert.cpp version.cpp graphics.cpp tiledata.cpp compress.cpp colors.cpp fileio.cpp colorquant.cpp options.cpp
LIBS        =-lz -lsquish -limagequant
OBJECTS     = $(SOURCES:.cpp=.o)
EXECUTABLE  = tileconv
ifeq ($(OS),Windows_NT)
  RM        = del
  LDFLAGS   += -static
else
  RM        = rm
  # Mac OS X does not support static linking
  ifneq ($(shell uname -s),Darwin)
    LDFLAGS   += -static
  endif
endif

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	$(RM) *.o
