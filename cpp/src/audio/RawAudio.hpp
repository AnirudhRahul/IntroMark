#ifndef DEFINED_RAWAUDIO_HPP
#define DEFINED_RAWAUDIO_HPP
#include <vector>
using std::vector;

struct TimeRange{
    double start;
    double end;
};

inline bool sortByStart(TimeRange a, TimeRange b){return a.start < b.start;}

struct RawAudio{
    int16_t* arr;
    vector<TimeRange> silence;
    char* filename;
    int sample_rate;
    int channels;
    int length;
    double lengthSec;
    bool deleted;
};
void freeRawAudio(RawAudio* input);
constexpr double silenceThreshold = 0.5;
RawAudio audioFileToArr(char * path);
int getCommonPrefix(RawAudio audioA, RawAudio audioB);
int getCommonSuffix(RawAudio audioA, RawAudio audioB);


#endif
