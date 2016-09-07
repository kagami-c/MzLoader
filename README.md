# MzLoader

A fast and lightweight mass spectrometry data importing library.

MzLoader is specific designed for ms2 data importing. Thus some fields in mass spectra
will not be extracted and some spectra will be skipped in order to accelerating the
loading speed.

The source code is tested in MSVC. Some adjustments on Makefile are needed to use this
package under Linux/UNIX environment.

## Dependencies

- RapidXML
- zlib
- libb64
- GoogleTest (optional)

All these stuff are in 3rdparty/ folder.

## License
BSD License
