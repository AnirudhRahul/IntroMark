#include <chromaprint.h>
#include <AudioFile.h>
#include <karkkainen_sanders.hpp>
#include <linear_longest_substring.hpp>
#include <iostream>
#include <math.h>
#include <algorithm>
#include <map>
#include <vector>
#include <cstring>
#include <coroutine.hpp>
#include <generator.hpp>

using std::cout; using std::endl; using std::sort; using std::tuple; using std::vector;

// Assert macro from https://stackoverflow.com/questions/3767869/adding-message-to-assert
#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            exit(EXIT_FAILURE); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) do { } while (false)
#endif

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

struct TimeRange{
    double start;
    double end;
};
bool sortByStart(TimeRange a, TimeRange b){return a.start < b.start;}

struct RawAudio{
    int16_t* arr;
    vector<TimeRange> silence;
    char* filename;
    int sample_rate;
    int channels;
    int length;
    double lengthSec;
};

double silenceThreshold = 0.5;
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

    return (struct RawAudio){arr, silentRanges, path, sample_rate, channels, samples*channels, (double)samples/sample_rate};
}
void freeAudio(RawAudio input){
    delete[] input.arr;
    input.silence.clear();
}

int getCommonPrefix(RawAudio audioA, RawAudio audioB){
    for(int i=0; i<std::min(audioA.length, audioB.length); i++){
        if(audioA.arr[i]!=audioB.arr[i])
            return i/audioA.channels;
    }
    return -1;
}
int getCommonSuffix(RawAudio audioA, RawAudio audioB){
    int tailA = audioA.length-1; int tailB = audioB.length-1;
    int len = 0;
    while(tailA>=0 && tailB>=0 && audioA.arr[tailA] == audioB.arr[tailB]){
        tailA--; tailB--; len++;
    }
    return len/audioA.channels;
}


struct ChromaArr{
    uint32_t* arr;
    int size;
};
// Only reads 2 files at a time to lower memory usage
cppcoro::generator<vector<TimeRange>> findSubstrings(vector<char*> pathList, bool verbose = false){

    int renewIndex = -1;
    RawAudio audioList[2]; ChromaArr chroma[2];
    int channels, sample_rate;
    ChromaprintContext *ctx;
    int delay=-1; int item_duration=-1;

    for(int pathIndex=1; pathIndex < pathList.size(); pathIndex++){
        if(renewIndex<0){
            audioList[0] = audioFileToArr(pathList[pathIndex-1]); audioList[1] = audioFileToArr(pathList[pathIndex]);
            channels = audioList[0].channels; sample_rate = audioList[0].sample_rate;
        }
        else{
            audioList[renewIndex] = audioFileToArr(pathList[pathIndex]);
        }
        for(int k=0;k<2;k++){
            ASSERT(channels == audioList[k].channels, "Tracks must have the same number of channels");
            ASSERT(sample_rate == audioList[k].sample_rate, "Tracks must have the same sample rate");
        }

        int startShift = getCommonPrefix(audioList[0], audioList[1]); ASSERT(startShift!=-1, "Audio files are the same");
        int endShift = getCommonSuffix(audioList[0], audioList[1]);
        double startShiftsec = (double) startShift/sample_rate; double endShiftsec = (double) endShift/sample_rate;
        cout << "START Shift " << startShiftsec << endl << "END Shift " << endShiftsec << endl;
        
        for(int k=0; k<2; k++){
            if(renewIndex==k || renewIndex<0){
                ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_TEST5, sample_rate);
                chromaprint_start(ctx, sample_rate, channels);
                chromaprint_feed(ctx, (audioList[k].arr + startShift*channels), audioList[k].length - (startShift+endShift)*channels);
                chromaprint_finish(ctx);
                if(renewIndex==k || (renewIndex<0 && k==0))
                    freeAudio(audioList[k]);
                if(delay<0){
                    delay = chromaprint_get_delay(ctx);
                    item_duration = chromaprint_get_item_duration(ctx);
                }
                ASSERT(delay == chromaprint_get_delay(ctx), "Delay Mismatch\n");
                ASSERT(item_duration == chromaprint_get_item_duration(ctx), "Item Duration Mismatch\n");
                
                chroma[k]=(struct ChromaArr){nullptr, 0};
                chromaprint_get_raw_fingerprint(ctx, &chroma[k].arr, &chroma[k].size);
                chromaprint_free(ctx);
            }
        }
        cout << "Chromaprint done!\n";

        int combinedLen = chroma[0].size + chroma[1].size + 1;
        int offset = chroma[0].size + 1;
        uint32_t * merged = new uint32_t[combinedLen];
        std::copy(chroma[0].arr, chroma[0].arr + chroma[0].size, merged);
        std::copy(chroma[1].arr, chroma[1].arr + chroma[1].size, merged + offset);
        delete[] chroma[(renewIndex+1)%2].arr;

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
        delete[] rankArr;
        cout << "Made LCP array\n";

        auto compareIndices = [compressed, rank_to_val](int a, int b) {
            return compare_gray_codes(rank_to_val[compressed[a]], rank_to_val[compressed[b]]);
        };
        auto toSec = [item_duration, sample_rate](int in) {
            return (double) in * item_duration / sample_rate;
        };

        int threshold = (int) (toSec(1));
        vector<CommonSubArr> common_substring_list = longest_common_substring(suffixArr, lcpArr, combinedLen, chroma[0].size, threshold);
        delete[] suffixArr;
        delete[] lcpArr;
        cout << "OLD LEN " << common_substring_list.size() << endl;
        
        int delay_item = delay/item_duration;
        int mergeThreshold = 5*delay_item;
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
                        // cout << match_measure/gap << " " << cur.startA << " " << next.startA << endl;
                        if(match_measure/gap < 0.75){
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
        delete[] compressed; delete[] rank_to_val;
        cout << "MID LEN  " << common_substring_list.size() << endl;
        common_substring_list.erase(
        std::remove_if(common_substring_list.begin(), common_substring_list.end(),
            [delay_item](const CommonSubArr & o) { return o.length <= delay_item; }),
        common_substring_list.end());

        cout << "NEW LEN " << common_substring_list.size() << endl; 

        // Add delay
        for(CommonSubArr common: common_substring_list){
            common.length += delay_item;
            int endA = common.startA + common.length;
            int endB = common.startB + common.length;
            if(endA >= chroma[0].size || endB >= combinedLen){
                continue;
            }
        }

        vector<TimeRange> outputRanges[2];
        for(int i=0;i<2;i++)
            outputRanges[i].push_back((struct TimeRange){0, startShiftsec}); 

        for(CommonSubArr common: common_substring_list){
            TimeRange curA = (struct TimeRange){
                startShiftsec + toSec(common.startA),
                startShiftsec + toSec(common.startA + common.length) + 8
            };
            outputRanges[0].push_back(curA);

            TimeRange curB = (struct TimeRange){
                startShiftsec + toSec(common.startB - offset),
                startShiftsec + toSec(common.startB - offset + common.length) + 8
            };
            outputRanges[1].push_back(curB);
        }
        for(int i=0;i<2;i++){
            outputRanges[i].push_back((struct TimeRange){audioList[i].lengthSec - endShiftsec, audioList[i].lengthSec});
        }
        
        // if(renewIndex<0){
        //     co_yield outputRanges[0];
        //     co_yield outputRanges[1];
        // }
        // else{
        //     co_yield outputRanges[renewIndex];
        // }

        renewIndex = (renewIndex+1)%2;
    }
}

/*
Command line options
    -f output to file
    -v verbose logs

The rest of the arguements should be a list of files in the order you want them compared.

*/
int main(int argc, char* argv[])
{
    vector<char*> pathList;
    char* outputFile;
    bool fileOutput = false;
    bool verbose = false;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"-f") || !strcmp(argv[i],"--file")){
            if(i+1>=argc){
                cout << "Must specify a filename when using -f\n";
                return EXIT_FAILURE;
            }
            i++;
            fileOutput = true;
            outputFile = argv[i];
        }
        else if(!strcmp(argv[i],"-v") || !strcmp(argv[i],"--verbose")){
            verbose = true;
        }
        else{
            pathList.push_back(argv[i]);
        }
    }

    if(pathList.size()<2){
        cout << "Not enough paths specified";
        return EXIT_FAILURE;
    }

    int index = 0;
    // for(vector<TimeRange> a:findSubstrings(pathList, verbose)){}
    for(vector<TimeRange> common : findSubstrings(pathList, verbose)){
        cout << pathList[index] << endl;
        for(TimeRange cur:common)
            cout << cur.start << " to " << cur.end << endl;
        index++;
    }
    cout << "PROCESSED " << index;

    // const char* path1 = "../../test_audio/hori1.wav";
    // const char* path2 = "../../test_audio/hori3.wav";
    
    return 0;
}
