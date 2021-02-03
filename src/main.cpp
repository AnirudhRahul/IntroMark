#include <chromaprint.h>
#include <AudioFile.h>
#include <karkkainen_sanders.hpp>
#include <linear_longest_substring.hpp>
#include <iostream>
#include <math.h>
#include <algorithm>
#include <map>

using std::cout; using std::endl; using std::sort; using std::tuple;

// Assert macro from https://stackoverflow.com/questions/3767869/adding-message-to-assert
#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) do { } while (false)
#endif

struct RawAudio{
    int16_t* arr;
    int sample_rate;
    int channels;
    int length;
};

struct ChromaArr{
    uint32_t* arr;
    int size;
};

tuple<int*, uint32_t*, int> compress(uint32_t* arr, int size){
    uint32_t* sorted = new uint32_t[size];
    std::copy_n(arr, size, sorted);
    sort(sorted, sorted+size);
    std::map<uint32_t, int> rankMap;
    // Rank 0 is reserved for sentinels
    int rank = 2;
    for(uint32_t i = 0; i < size; i++){
        if(i==0 || sorted[i]!=sorted[i-1]){
            rankMap[sorted[i]] = rank;
            rank++;
        }
    }
    delete[] sorted;
    int* res = new int[size+3];
    res[size] = res[size+1] = res[size+2] = 0;
    for(int i=0; i < size; i++){
        res[i] = rankMap[arr[i]];
    }
    delete[] arr;

    uint32_t* rank_to_val = new uint32_t[rank];
    for (auto const& x : rankMap){
        rank_to_val[x.second] = x.first;
    }

    rankMap.clear();
    return std::make_tuple(res, rank_to_val, rank);
}

double compare_gray_codes(uint32_t a, uint32_t b){
    uint32_t c = a ^ b;
    double matches = 0;
    for(int i=0; i<16; i++){
        if( (c&3) == 0)
            matches+=1;
        c>>=2;
    }

    return matches/16;
}

RawAudio audioFileToArr(const char * path){
    AudioFile<float> audioFile;
    audioFile.load(path);
    
    int samples = audioFile.getNumSamplesPerChannel();
    int channels = audioFile.getNumChannels();

    int16_t* arr = new int16_t[samples*channels];

    int index = 0;
    for (int i = 0; i < samples; i++){
        for(int j = 0; j < channels; j++){
            arr[index] = round(audioFile.samples[j][i] * 32767); 
            index++;
        }
    }

    return (struct RawAudio){arr, (int)audioFile.getSampleRate(), channels, samples*channels};
}

void freeAudio(RawAudio input){
    delete[] input.arr;
}

int getCommonStart(int16_t* a, int16_t* b, int sizeA, int sizeB, int channels){
    for(int i=0; i<std::min(sizeA, sizeB); i++){
        if(a[i]!=b[i])
            return i/channels;
    }
    return -1;
}

int getCommonEnd(int16_t* a, int16_t* b, int sizeA, int sizeB, int channels){
    int tailA = sizeA-1; int tailB = sizeB-1;
    int len = 0;
    while(tailA>=0 && tailB>=0 && a[tailA] == b[tailB]){
        tailA--; tailB--; len++;
    }
    return len/channels;
}

int main()
{
    const char* path1 = "../test_audio/normal_ep1.wav";
    const char* path2 = "../test_audio/normal_ep2.wav";
    RawAudio audio1 = audioFileToArr(path1);
    RawAudio audio2 = audioFileToArr(path2);
    cout << "Read audio\n";

    int channels = audio1.channels;
    ASSERT(channels == audio2.channels, "Tracks must have the same number of channels");
    int sample_rate = audio1.sample_rate;
    ASSERT(sample_rate == audio2.sample_rate, "Tracks must have the same sample rate");

    int startShift = getCommonStart(audio1.arr, audio2.arr, audio1.length, audio2.length, channels);
    ASSERT(startShift!=-1, "Audio files are the same");
    int endShift = getCommonEnd(audio1.arr, audio2.arr, audio1.length, audio2.length, channels);
    double startShiftsec = (double) startShift/sample_rate;
    cout << "START Shift " << startShiftsec << endl;
    double endShiftsec = (double) endShift/sample_rate;
    cout << "END Shift " << endShiftsec << endl;



    ChromaArr chroma[2];
    RawAudio audioList[2] = {audio1, audio2};
    int delay, item_duration;

    ChromaprintContext *ctx;
    int index = 0;
    for(RawAudio cur : audioList){
        ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_TEST5);
        chromaprint_start(ctx, cur.sample_rate, cur.channels);
        chromaprint_feed(ctx, (cur.arr + startShift*channels), cur.length - (startShift+endShift)*channels);
        freeAudio(cur);
        chromaprint_finish(ctx);
        if(index==0){
            delay = chromaprint_get_delay(ctx);
            item_duration = chromaprint_get_item_duration(ctx);
        }
        else{
            ASSERT(delay == chromaprint_get_delay(ctx), "Delay Mismatch\n");
            ASSERT(item_duration == chromaprint_get_item_duration(ctx), "Item Duration Mismatch\n");
        }
        chroma[index]=(struct ChromaArr){nullptr, 0};
        chromaprint_get_raw_fingerprint(ctx, &chroma[index].arr, &chroma[index].size);
        chromaprint_free(ctx);
        index++;
    }

    cout << "Chromaprint done!\n";

    int combinedLen = chroma[0].size + chroma[1].size + 1;
    int offset = chroma[0].size + 1;
    uint32_t * merged = new uint32_t[combinedLen];
    std::copy(chroma[0].arr, chroma[0].arr + chroma[0].size, merged);
    delete[] chroma[0].arr;
    std::copy(chroma[1].arr, chroma[1].arr + chroma[1].size, merged + offset);
    delete[] chroma[1].arr;

    int max; int* compressed; uint32_t* rank_to_val;
    std::tie(compressed, rank_to_val, max) = compress(merged, combinedLen);
    // Sentinel in between the 2 strings
    compressed[chroma[0].size] = 0;
    cout << "Finished compressing\n";

    int* suffixArr = karkkainen_sanders_sa(compressed, combinedLen, max);
    cout << "Made suffix array\n";

    int* rankArr = create_rank_arr(suffixArr, combinedLen);
    // lcp in range from [1, combinedLen)
    int* lcpArr =  create_lcp_arr(suffixArr, rankArr, compressed, combinedLen);
    cout << "Made LCP array\n";

    int threshold = (int) (1.0 * sample_rate / item_duration);
    CommonSubArr common = longest_common_substring(suffixArr, lcpArr, combinedLen, chroma[0].size, threshold)[0];
    cout << "Found least common substring\n";

    auto toSec = [item_duration, sample_rate](int in) {
        return (double) in * item_duration / sample_rate;
    };
    auto compareIndices = [compressed, rank_to_val](int a, int b) {
        return compare_gray_codes(rank_to_val[compressed[a]], rank_to_val[compressed[b]]);
    };

    double delay_sec = (double) delay / sample_rate;
    int sizeA = chroma[0].size;
    cout << "ENDOH: " << toSec(sizeA) + delay_sec << endl;

    cout << endl << "Common Substrings found" << endl;
    cout << "Length in sec: " << toSec(common.length) + delay_sec << endl;
    cout << startShiftsec + toSec(common.startA) << " to " << startShiftsec + toSec(common.startA + common.length) + delay_sec << endl;
    cout << startShiftsec + toSec(common.startB - sizeA - 1) << " to " << startShiftsec + toSec(common.startB + common.length - sizeA - 1) + delay_sec << endl;
    cout << endl;

    int startA = common.startA; int startB = common.startB;
    const int mismatch_threshold = 1;
    int mismatch_count=0;
    while(startA>=0 && startB>=offset){
        if(compareIndices(startA, startB)<0.8)
            mismatch_count++;
        if(mismatch_count > mismatch_threshold)
            break;
        startA--;startB--;
    }
    while(compressed[startA]!=compressed[startB]){
        startA++; startB++;
    }
    mismatch_count = 0;
    int endA = startA + common.length; int endB = startB + common.length;
    endA-=1; endB-=1;

    while(endA<sizeA && endB<combinedLen){
        if(compareIndices(endA, endB)<0.8)
            mismatch_count++;
        if(mismatch_count > mismatch_threshold)
            break;
        endA++;endB++;
    }
    while(compressed[endA]!=compressed[endB]){
        endA--; endB--;
    }

    cout << startShiftsec + toSec(startA) << " to " << startShiftsec + toSec(endA) + delay_sec << endl;
    cout << startShiftsec + toSec(startB - sizeA - 1) << " to " << startShiftsec+ toSec(endB - sizeA - 1) + delay_sec << endl;
    cout << "NEW LEN: " << toSec(endA-startA) + delay_sec << endl;

    cout << "DELAY: " << delay_sec << endl;
    cout << "SINGLE SAMPLE LENGTH: " << toSec(1) << endl;
    // for(int i=startA-10; i<endA; i++){
    //     cout << compressed[i] << " ";
    // }
    // cout << "\n\n";
    // for(int i=startB-10; i<endB; i++){
    //     cout << compressed[i] << " ";
    // }
    cout << "\n\n";

    return 0;
}
