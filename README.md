This tool allows you to compress or decompress TIS and MOS files provided by Infinity Engine games such as Baldur's Gate or Icewind Dale.
The tool uses modern compression algorithms to achieve great compression ratios without noticeable degradation in quality.

**Supported conversions:**
- TIS files will be automatically compressed into the TBC format. 
- TBC files will be automatically decompressed into the TIS format.
- MOS files will be automatically compressed into the MBC format.
- MBC files will be automatically decompressed into the MOS format.

A detailed description of the TBC and MBC formats can be found in FORMATS.


### USAGE
```
Usage: tileconv [options] infile [infile2 [...]]

Options:
 -e          Do not halt on errors.
 -s          Be silent.
 -v          Be verbose.
 -t type     Select pixel encoding type.
             Supported types:
               0: No pixel encoding
               1: BC1/DXT1 (Default)
               2: BC2/DXT3
               3: BC3/DXT5
 -u          Do not apply tile compression.
 -o outfile  Select output file. (Works with single input file only!)
 -d          Use color dithering when converting to TIS or MOS.
 -z          MOS only: Decompress MBC into compressed MOS (MOSC).
 -V          Print version number and exit.

Supported input file types: TIS, MOS, TBC, MBC
Note: You can mix and match input files of each supported type.
```


### LICENSE

tileconv is distributed under the terms and conditions of the MIT license.
This license is specified at the top of each source file and must be preserved 
in its entirety.


### BUILDING TILECONV
**Dependencies:**
- zlib (http://www.zlib.net/)
- libsquish (https://code.google.com/p/libsquish/)
- pngquant (https://github.com/pornel/pngquant/)

External libraries and include files are assumed to be located in the 
subfolders "zlib", "squish" and "pngquant". The libraries are providing their 
own instructions how to compile them. Afterwards call "make" to build tileconv.

If you want to change paths for the external libraries or include files, 
you can do so by modifying the file "config.mk" by hand.


### CONTACT
If you have questions or comments please post them on [Spellhold Studios](http://www.shsforums.net/) or contact me (Argent77) by private message on the same forum.
