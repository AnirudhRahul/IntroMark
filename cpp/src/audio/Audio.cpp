#include "AudioFile.h"
#include "Audio.hpp"
#include <vector>
#include <iostream>
#include <algorithm>
#include<fstream>
#include <chromaprint.h>
using std::vector, std::cout;
namespace fs = std::filesystem;

Audio::Audio(){
    chroma = new int[1];
    filename = "NULL";
}

double frame_shift_sec = 1.0/24.0;
double frame_size_sec = 12.0;

Audio::Audio(char *path){
    filename=path;
//    auto chromaFilename = fs::path(filename).replace_extension("chroma4").string();
//    if(fs::exists(chromaFilename)){
//        cout << "Reading from chroma dump: "<<chromaFilename<<"\n";
//        std::ifstream fstream(chromaFilename, std::ios_base::binary);
//        fstream.read((char*)(this), sizeof(Audio));
//        chroma = new uint32_t[chromaLength];
//        fstream.read((char*)(chroma), sizeof(uint32_t)*chromaLength);
//        filename = chromaFilename.c_str();
//        fstream.close();
//        return;
//    }
    
    AudioFile<float> audioFile;
    if(!audioFile.load(path)){
        exit (EXIT_FAILURE);
    }
    cout << "Processing " << path << " ...\n";
    channels = audioFile.getNumChannels();
    sample_rate = (int)audioFile.getSampleRate();
    samples = audioFile.getNumSamplesPerChannel();
    lengthSec = (double)samples/sample_rate;
    
    vector<long long> out;
    int frame_size = frame_size_sec * sample_rate;
    int frame_shift = frame_shift_sec * sample_rate;
    long long sum=0;
    for(int i=0;i<frame_size;i++){
        sum+= std::abs(audioFile.samples[0][i] + audioFile.samples[1][i])/2*255;
    }
    out.push_back(sum);
    int start = frame_shift;
    while(start+frame_size<=samples){
        
        for(int i=1;i<=frame_shift;i++){
            sum-= std::abs(audioFile.samples[0][start-i] + audioFile.samples[1][start-i])/2*255;
            
            sum+= std::abs(audioFile.samples[0][start+frame_size-i] + audioFile.samples[1][start+frame_size-i])/2*255;

        }
        out.push_back(sum);
        start+=frame_shift;
        
    }
    
    
    int* ratio = new int[out.size()];
    ratio[0]=0;
    for(int i=1;i<out.size();i++){
        int clamp1 = std::clamp((int)(std::log(out[i])*1000), -10*1000, 10*1000);
        int clamp2 = std::clamp((int)(std::log(out[i-1])*1000), -10*1000, 10*1000);


        int res = (clamp1-clamp2)/1000;
        ratio[i]=res;
    }
    std::sort(ratio, ratio+out.size());
    cout << "BIG\n";
    for(int i=1;i<=100;i++){
        cout << ratio[out.size()-i] << " ";
    }
    cout << "SMOL\n";
    for(int i=0;i<=100;i++){
        cout << ratio[i] << " ";
    }
    
    cout << "\n";
    
    cout << "Length sec " << lengthSec << "\n";
    cout << "Predicted len " << (samples-frame_size)/frame_shift+1 << "\n";
    cout << "Real len " << out.size() << "\n";
    chroma = ratio;
    chromaLength = out.size();
    
//    ChromaprintContext *ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_TEST5, sample_rate);
//    chromaprint_start(ctx, sample_rate, 1);
//    item_duration = chromaprint_get_item_duration(ctx);
//    delay = chromaprint_get_delay(ctx);
//
//    int chunk_size = 1024;
//    int16_t* buffer = new int16_t[chunk_size];
//    float sum = 0;
//    int bufferIndex = 0;
//    int last_percent_reported = 0;
//    for (int i = 0; i < samples; i++){
//        for(int j = 0; j < channels; j++){
//            sum += audioFile.samples[j][i];
//        }
//        // Average out channels to convert to mono(this is done by chromaprint anyway)
//        buffer[bufferIndex] = round((sum / channels) * 32767);
//        bufferIndex++;
//        if(bufferIndex==1024){
//            chromaprint_feed(ctx, buffer, chunk_size);
//            bufferIndex=0;
//        }
//
//        double progress = (double)i/samples;
//        if(progress*10000 > last_percent_reported){
//            last_percent_reported = (int)(progress*10000)+1;
//            cout << "\rProgress: " << last_percent_reported/100 <<"."<< last_percent_reported%100 << "% ";
//            cout.flush();
//        }
//    }
//    cout << "\n";
//    if(bufferIndex!=0)
//        chromaprint_feed(ctx, buffer, bufferIndex);
//    delete[] buffer;
    
//    chromaprint_finish(ctx);
//    chromaprint_get_raw_fingerprint(ctx, &chroma, &chromaLength);
//    chromaprint_free(ctx);
//
//    std::ofstream fstream(chromaFilename, std::ios_base::binary | std::ios_base::out);
//    fstream.write((char*)(this), sizeof(Audio));
//    fstream.write((char*)chroma, sizeof(uint32_t)*chromaLength);
//    fstream.flush();
//    fstream.close();
//    cout << "Saved to file: "<<chromaFilename<<"\n";
    
    
    
    
}

Audio::~Audio(){
//    delete[] chroma;
}
