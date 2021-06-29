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
    Audio(char* path);
    ~Audio();
    uint32_t* chroma;
    int chromaLength;
    const char* filename;
    int sample_rate;
    int channels;
    int samples;
    double lengthSec;
};

 void freeAudio(Audio* input);
//Audio audioFileToArr(char * path);


#endif
