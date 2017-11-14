# piha

Driver library written in C for the SEPS525.

Current status: Running picture frame program on display using a Raspberry PI Zero
To get the picture frame to work: Drop a bunch of  ppm images in `build/images` with the exact resolution of 160x128

##### Todo
* Move display driver code to libpiha
* Move picture frame program to examples
* Using [libmgba](https://mgba.io/), create a tiny hardware gameboy device


## Building
```sh
mkdir build && cd build
cmake .. && make
./display-test
```

### Dependencies

wiringPi


## Development


### Newhaven resources
* [SEPS525 Display](http://www.newhavendisplay.com/app_notes/SEPS525.pdf)
* [NHD-1.69-160128UGC3](http://www.newhavendisplay.com/specs/NHD-1.69-160128UGC3.pdf)
* [Newhaven example code](https://github.com/NewhavenDisplay/NHD-1.69-160128UGC3_Example/)
