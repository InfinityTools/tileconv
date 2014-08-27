#########################
# Makefile for tileconv #
#########################

include config.mk

CXX         ?= g++
CXXFLAGS    = -c -Wall -O2 -std=c++11 -I$(ZLIB_INCLUDE) -I$(SQUISH_INCLUDE) -I$(PNGQUANT_INCLUDE)
LDFLAGS     = -static -L$(ZLIB_LIB) -L$(SQUISH_LIB) -L$(PNGQUANT_LIB)
SOURCES     = tileconv.cpp convert.cpp version.cpp graphics.cpp tiledata.cpp compress.cpp colors.cpp fileio.cpp colorquant.cpp options.cpp
LIBS        =-lz -lsquish -limagequant
OBJECTS     = $(SOURCES:.cpp=.o)
EXECUTABLE  = tileconv
ifeq ($(OS),Windows_NT)
  RM        = del
else
  RM        = rm
endif

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	$(RM) *.o
