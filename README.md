# IntroMark
IntroMark allows you to automatically mark repeating segments in video or audio files.

# Dependencies
Note installing fftw3 is optional, but it does make IntroMark run much faster.

Ubuntu:
```
apt-get install libfftw3-dev
```

MacOS:
```
brew install fftw
```

# How to build
Note that IntroMark makes use of the coroutine feature from c++20, so you will need to use a newer compiler which supports that.

Compilers that I have found to work are
* Apple Clang 12.0.0 on MacOS
* gcc-8/g++-8 (Ubuntu 8.4.0-1ubuntu1~18.04) 8.4.0 on WSL Ubuntu 18.04
  * [Instructions for installing gcc-8 on ubuntu](https://askubuntu.com/a/1087116/1171839)


Then move into the build folder ```cd build```

If you installed fftw3 run ```cmake .. -DFFT_LIB=fftw3```

Or use kissFFT ```cmake .. -DFFT_LIB=kissfft```

Note if you see the warning
```
 Target "IntroMark" requires the language dialect "CXX20" (with compiler
  extensions), but CMake does not know the compile flags to use to enable it.
```
You may need to specify which compiler cmake should use:
```
cmake -D CMAKE_C_COMPILER=gcc-8 -D CMAKE_CXX_COMPILER=g++-8 -DFFT_LIB=fftw3 ..
```





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
