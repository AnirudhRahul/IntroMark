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

# Supported Compilers

* Apple Clang 12.0.0 on MacOS
* gcc-8/g++-8 (Ubuntu 8.4.0-1ubuntu1~18.04) 8.4.0 on WSL Ubuntu 18.04
  * [Instructions for installing gcc-8 on ubuntu](https://askubuntu.com/a/1087116/1171839)

# How to build

Clone the repo ``` git clone https://github.com/AnirudhRahul/IntroMark.git ```

Then move into the build folder ```cd cpp/build```

If you installed fftw3 run ```cmake .. -DFFT_LIB=fftw3```

Else use kissFFT(around 2-3x slower) ```cmake .. -DFFT_LIB=kissfft```

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
make && ./IntroMark file1 file2
```

Note binary can be used from the command line with the following format

```./Intromark [files]```

Where ```[files]``` is a space seperated list of audio files
