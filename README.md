# IntroMark
IntroMark allows you to automatically mark repeating segments in video or audio files.

# How to build
Note installing fftw3 is optional, but it does make IntroMark run much faster.

Ubuntu:
```
apt-get install libfftw3-dev
```

MacOS:
```
brew install fftw
```



Then move into the build folder ```cd build```
If you installed fftw3 run ```cmake .. -DFFT_LIB=fftw3```
Or use kissFFT ```cmake .. -DFFT_LIB=kissfft```

Then build and run the binary with:
```
make && ./IntroMark
```


# How to compile
If you don't have the `build` directory, you need to create it (`mkdir build`).

Optional this project can get a 2-3x speedup if you install fftw3.
On ubuntu you can simply use:


In the `build` directory do those commands that will compile the sources with chromaprint linked:
```bash
cmake ..
cmake --build .
```
