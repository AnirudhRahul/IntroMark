#include "AudioFile.h"
#include "RawAudio.hpp"
#include <vector>
using std::vector;

void freeRawAudio(RawAudio* input){
    if(!input->deleted){
        delete[] input->arr;
        input->silence.clear();
        input->deleted = true;
    }
}

RawAudio audioFileToArr(char * path){
    AudioFile<float> audioFile;
    if(!audioFile.load(path)){
        exit (EXIT_FAILURE);
    }
    
    int samples = audioFile.getNumSamplesPerChannel();
    int channels = audioFile.getNumChannels();
    int sample_rate = (int)audioFile.getSampleRate();

    int16_t* arr = new int16_t[samples*channels];
    vector<TimeRange> silentRanges;

    int index = 0;
    int silenceStart=0; int silenceEnd=0;
    for (int i = 0; i < samples; i++){
        double volume = 0;
        for(int j = 0; j < channels; j++){
            volume += abs(audioFile.samples[j][i]);
            arr[index] = round(audioFile.samples[j][i] * 32767); 
            index++;
        }
        silenceEnd++;
        if(volume/channels > 0.01){
            if(silenceEnd - silenceStart > sample_rate*silenceThreshold){
                silentRanges.push_back((struct TimeRange){
                    (double) silenceStart/sample_rate, 
                    (double) silenceEnd/sample_rate 
                });
            }
            silenceStart = silenceEnd;
        }
    }

    return (struct RawAudio){arr, silentRanges, path, sample_rate, channels, samples*channels, (double)samples/sample_rate, false};
}

int getCommonPrefix(RawAudio audioA, RawAudio audioB){
    for(int i=0; i<std::min(audioA.length, audioB.length); i++){
        if(audioA.arr[i]!=audioB.arr[i])
            return i/audioA.channels;
    }
    return std::min(audioA.length, audioB.length)/audioA.channels;
}
int getCommonSuffix(RawAudio audioA, RawAudio audioB){
    int tailA = audioA.length-1; int tailB = audioB.length-1;
    int len = 0;
    while(tailA>=0 && tailB>=0 && audioA.arr[tailA] == audioB.arr[tailB]){
        tailA--; tailB--; len++;
    }
    return len/audioA.channels;
}