#ifndef DEFINED_Audio_HPP
#define DEFINED_Audio_HPP
#include <vector>
using std::vector;

struct TimeRange{
    double start;
    double end;
};

inline bool sortByStart(TimeRange a, TimeRange b){return a.start < b.start;}

// struct hold for holding audio data
// all audio is converted to mono
struct Audio{
    Audio(char* path, int f, int overlap, int c, int w);
    Audio();
    ~Audio();
    uint32_t* chroma;
    int chromaLength;
    int delay;
    int item_dur;
    const char* filename;
    int sample_rate;
    int channels;
    int samples;
    double lengthSec;
};

#endif
