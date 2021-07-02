#include "AudioFile.h"
#include "Audio.hpp"
#include <vector>
#include <iostream>
#include<fstream>
#include <chromaprint.h>
using std::vector, std::cout;
namespace fs = std::filesystem;

Audio::Audio(){
    chroma = new uint32_t[1];
    filename = "NULL";
}

Audio::Audio(char *path){
    filename=path;
    auto chromaFilename = fs::path(filename).replace_extension("chroma4").string();
    if(fs::exists(chromaFilename)){
        cout << "Reading from chroma dump: "<<chromaFilename<<"\n";
        std::ifstream fstream(chromaFilename, std::ios_base::binary);
        fstream.read((char*)(this), sizeof(Audio));
        chroma = new uint32_t[chromaLength];
        fstream.read((char*)(chroma), sizeof(uint32_t)*chromaLength);
        filename = chromaFilename.c_str();
        fstream.close();
        return;
    }
    
    AudioFile<float> audioFile;
    if(!audioFile.load(path)){
        exit (EXIT_FAILURE);
    }
    cout << "Processing " << path << " ...\n";
    channels = audioFile.getNumChannels();
    sample_rate = (int)audioFile.getSampleRate();
    samples = audioFile.getNumSamplesPerChannel();
    lengthSec = (double)samples/sample_rate;
    
    ChromaprintContext *ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_TEST5, sample_rate);
    chromaprint_start(ctx, sample_rate, 1);
    item_duration = chromaprint_get_item_duration(ctx);
    delay = chromaprint_get_delay(ctx);
    
    int chunk_size = 1024;
    int16_t* buffer = new int16_t[chunk_size];
    float sum = 0;
    int bufferIndex = 0;
    int last_percent_reported = 0;
    for (int i = 0; i < samples; i++){
        for(int j = 0; j < channels; j++){
            sum += audioFile.samples[j][i];
        }
        // Average out channels to convert to mono(this is done by chromaprint anyway)
        buffer[bufferIndex] = round((sum / channels) * 32767);
        bufferIndex++;
        if(bufferIndex==1024){
            chromaprint_feed(ctx, buffer, chunk_size);
            bufferIndex=0;
        }
        
        double progress = (double)i/samples;
        if(progress*10000 > last_percent_reported){
            last_percent_reported = (int)(progress*10000)+1;
            cout << "\rProgress: " << last_percent_reported/100 <<"."<< last_percent_reported%100 << "% ";
            cout.flush();
        }
    }
    cout << "\n";
    if(bufferIndex!=0)
        chromaprint_feed(ctx, buffer, bufferIndex);
    delete[] buffer;
    
    chromaprint_finish(ctx);
    chromaprint_get_raw_fingerprint(ctx, &chroma, &chromaLength);
    chromaprint_free(ctx);
    
    std::ofstream fstream(chromaFilename, std::ios_base::binary | std::ios_base::out);
    fstream.write((char*)(this), sizeof(Audio));
    fstream.write((char*)chroma, sizeof(uint32_t)*chromaLength);
    fstream.flush();
    fstream.close();
    cout << "Saved to file: "<<chromaFilename<<"\n";
}

Audio::~Audio(){
    delete[] chroma;
}
