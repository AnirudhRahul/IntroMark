#include <chromaprint.h>
#include <AudioFile.h>
#include <karkkainen_sanders.hpp>
#include <linear_longest_substring.hpp>
#include <iostream>
#include <math.h>
#include <algorithm>
#include <map>
#include <vector>

using std::cout; using std::endl; using std::sort; using std::tuple; using std::vector;

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

    auto compareIndices = [compressed, rank_to_val](int a, int b) {
        return compare_gray_codes(rank_to_val[compressed[a]], rank_to_val[compressed[b]]);
    };
    auto toSec = [item_duration, sample_rate](int in) {
        return (double) in * item_duration / sample_rate;
    };

    int threshold = (int) (toSec(1));
    vector<CommonSubArr> common_substring_list = longest_common_substring(suffixArr, lcpArr, combinedLen, chroma[0].size, threshold);
    cout << "OLD LEN " << common_substring_list.size() << endl;
    
    int delay_item = delay/item_duration;
    int mergeThreshold = 2*delay_item;
    int offsetThreshold = std::max(2, (int)(0.1 * sample_rate / item_duration));
    for(int i=common_substring_list.size()-1;i>0;i--){
        CommonSubArr next = common_substring_list[i]; 
        for(int k=i-1; k>=0; k--){
            CommonSubArr cur = common_substring_list[k];
            int gap = next.startA - cur.startA - cur.length;
            if(gap<=mergeThreshold && abs(cur.startA - cur.startB - (next.startA - next.startB)) <= offsetThreshold){
                if(gap>0){
                    double match_measure = 0;
                    for(int j=1; j<=gap; j++){
                        match_measure+=compareIndices(next.startA-j, next.startB-j);
                    }
                    if(match_measure/gap < 0.7){
                        continue;
                    }
                }
                common_substring_list[i] = (struct CommonSubArr){
                    cur.startA,
                    cur.startB,
                    std::min(next.startA + next.length - cur.startA, next.startB + next.length - cur.startB)
                };
                common_substring_list.erase(common_substring_list.begin()+k);
                break;
            }
            if(next.startA-cur.startA>=mergeThreshold)
                break;
        }
    }
    // Filter out smaller because they're likely inaccurate
    for(int i = common_substring_list.size(); i>=0; i--){
        if(common_substring_list[i].length <= delay_item)
            common_substring_list.erase(common_substring_list.begin() + i);
    }
    cout << "NEW LEN " << common_substring_list.size() << endl; 

    // Add delay
    for(CommonSubArr common: common_substring_list){
        bool added = false;
        cout << common.startA + common.length + delay_item << " versus " << chroma[0].size << endl;
        cout << common.startB + common.length + delay_item << " versus " << combinedLen << endl;
        for(int i=0;i<delay_item;i++){
            int indexA = common.startA + common.length + i;
            int indexB = common.startB + common.length + i;
            cout << compareIndices(indexA, indexB) << ", ";
        }
        cout << endl;

        for(int i=delay_item;i>delay_item/4;i--){
            int indexA = common.startA + common.length + i;
            int indexB = common.startB + common.length + i;
            if(indexB>=combinedLen || indexA>=chroma[0].size){
                continue;
            }
            if(compareIndices(indexA, indexB) > 0.4){
                cout << "Matched " << i << " out of total " << delay_item << endl;
                common.length+=i;
                added=true;
                break;
            }
        }
        if(!added)
            common.length+=delay_item/4;
    }


    double delay_sec = (double) delay / sample_rate;
    double secsA = toSec(chroma[0].size) + delay_sec;
    double secsB = toSec(chroma[1].size) + delay_sec;
    cout << "END OF A: " << secsA << endl;
    cout << "END OF B: " << secsB << endl;

    struct TimeRange{
        double start;
        double end;
    };
    vector<TimeRange> listA; vector<TimeRange> listB;
    listA.push_back((struct TimeRange){0, startShiftsec}); 
    listB.push_back((struct TimeRange){0, startShiftsec});
    for(CommonSubArr common: common_substring_list){
        TimeRange curA = (struct TimeRange){
            startShiftsec + toSec(common.startA),
            startShiftsec + toSec(common.startA + common.length)
        };
        listA.push_back(curA);

        TimeRange curB = (struct TimeRange){
            startShiftsec + toSec(common.startB - offset),
            startShiftsec + toSec(common.startB - offset + common.length)
        };
        listB.push_back(curB);
    }
    listA.push_back((struct TimeRange){secsA - endShiftsec, secsA});
    listB.push_back((struct TimeRange){secsB - endShiftsec, secsB});

    cout << "DELAY: " << delay_sec << endl;
    cout << "SINGLE SAMPLE LENGTH: " << toSec(1) << endl;

    cout << "\n\n\n";

    cout << path1 << endl;
    for(TimeRange cur:listA){
        cout << cur.start << " to " << cur.end << endl;
    }

    cout << path2 << endl;
    for(TimeRange cur:listB){
        cout << cur.start << " to " << cur.end << endl;
    }

    return 0;
}
