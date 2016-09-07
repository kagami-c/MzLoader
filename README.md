# MzLoader

A fast and lightweight mass spectrometry data importing library.

MzLoader is specific designed for ms2 data importing. Thus some fields in mass spectra
will not be extracted and some spectra will be skipped in order to accelerating the
loading speed.

Makefile is written for MSVC and Clang. Please feel free to make adjustments on it.

## Usage

The basic usage is available in test/ folder. To run unittest, please change the directory
to test/ folder first.

## Dependencies

- RapidXML
- zlib
- libb64
- GoogleTest (optional)

All these stuff are in 3rdparty/ folder.

## License
BSD License
