# Chromaprint C++ Boilerplate
Start a [Chromaprint](https://github.com/acoustid/chromaprint) C++ project faster than ever!

# How to clone the repository
Since we are using [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules), the repository needs to be cloned in a specific way in order to fetch those submodules.

## With SSH
```bash
git clone --recursive git@github.com:quantumsheep/chromaprint-boilerplate.git
```

## Without SSH
```bash
git clone --recursive https://github.com/quantumsheep/chromaprint-boilerplate.git
```

# How to compile
If you don't have the `build` directory, you need to create it (`mkdir build`).

Optional this project can get a 2-3x speedup if you install fftw3.
On ubuntu you can simply use:

``` apt-get install libfftw3-dev ```

In the `build` directory do those commands that will compile the sources with chromaprint linked:
```bash
cmake ..
cmake --build .
```
