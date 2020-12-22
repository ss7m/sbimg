# sbimg

A simple image viewer for X. Currently only supports png files, hopefully
will support more image types in the near future. Takes a single file as input,
and allows you to navigate through all of the images in the folder containing that file.

## Installation
To build:
```
make release
```
Generated binary will be `~/release/sbimg`. To install:
```
make install
```
Note that by default, binary will be installed in `$HOME/.local/bin`. To
install in a different directory like `/usr/bin`:
```
make INSTALL_DIR=/usr/bin install
```

## Usage

```
Usage: sbimg [file]

A simple X image viewer

Options:
  -h, --help     print this message and exit

Controls:
  q              quit program
  h              move image left
  j              move image down
  k              move image up
  l              move image right
  H              go to previous image
  J              zoom out
  K              zoom in
  L              go to next image
```
