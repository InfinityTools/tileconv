# Where to install (Linux/Mac only)
INSTALL_DIR       ?= /usr/local

# Include paths of the external libraries
ZLIB_INCLUDE      ?= ./zlib
SQUISH_INCLUDE    ?= ./squish
PNGQUANT_INCLUDE  ?= ./pngquant
JPEG_INCLUDE      ?= ./jpeg-turbo

# Paths to the libraries
ZLIB_LIB          ?= ./zlib
SQUISH_LIB        ?= ./squish
PNGQUANT_LIB      ?= ./pngquant/lib
JPEG_LIB          ?= ./jpeg-turbo

# Set to 1 when compiling on Windows using Win32 threads
USE_WINTHREADS    ?= 0
