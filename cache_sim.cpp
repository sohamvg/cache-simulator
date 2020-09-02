#include <iostream>
#include <vector>

using namespace std;

#define MAIN_MEMORY_BLOCKS 1024
#define R 1
#define W 0
#define PRESENT 1
#define NOT_PRESENT 0
#define HIT 1
#define MISS 0
#define DEBUG 1

int cache_sim(int cache_size, int cache_block_size, int cache_associativity, int T, vector<vector<int>> &instructions)
{
    int total_cache_blocks = cache_size / cache_block_size;
    int total_sets = total_cache_blocks / cache_associativity;

    int main_memory[MAIN_MEMORY_BLOCKS] = {0}; // block addressable (block size same as cache_block_size)
    int cache_block_data[total_sets][cache_associativity] = {0}; // block addressable
    int cache_block_valid[total_sets][cache_associativity] = {NOT_PRESENT};
    int cache_block_tag[total_sets][cache_associativity] = {0};
    int cache_block_latest_access_time[total_sets][cache_associativity] = {0};
    int set_priority_divider[total_sets] = {0}; // = total lines in HIGH PRIORITY group = index of first line in LOW PRIORITY group

#if DEBUG
    cout << "total_cache_blocks: " << total_cache_blocks << endl;
    cout << "total_sets: " << total_sets << endl;

    cout << "******************** CACHE ***************************" << endl;
    for (int s = 0; s < total_sets; s++)
    {
        cout << "############################" << endl;
        cout << "Set: " << s << endl;
        cout << "set_priority_divider: " << set_priority_divider[s] << endl;

        for (int ca = 0; ca < cache_associativity; ca++)
        {
            cout << "----------------------------" << endl;
            cout << "ca: " << ca << endl;
            cout << "cache_block_valid: " << cache_block_valid[s][ca] << endl;
            cout << "cache_block_tag: " << cache_block_tag[s][ca] << endl;
            cout << "cache_block_latest_access_time: " << cache_block_latest_access_time[s][ca] << endl;

            cout << "cache_block_data: " << cache_block_data[s][ca] << endl;
        }
    }
    cout << "*****************************************************" << endl;

#endif

    int inst_count = 0;
    for (auto it = instructions.begin(); it != instructions.end(); it++)
    {
        vector<int> inst = *it;
        int memory_address = inst.at(0); // this is block address
        int inst_type = inst.at(1);

        /*
            If byte address is:     <tag><set_index><offset>
            then block address is:  <tag><set_index>
        */

        int set_index = memory_address % total_sets;
        int tag = memory_address / total_sets;

#if DEBUG
        cout << "=========================== instruction ==============================" << endl;
        cout << "inst_count: " << inst_count << endl;
        cout << "memory_address: " << memory_address << endl;
        cout << "inst_type: " << inst_type << endl;
        cout << "set_index: " << set_index << endl;
        cout << "tag: " << tag << endl;
#endif

        int hit_or_miss = MISS;

        if (inst_type == R)
        {
            int first_non_valid_index = -1;
            for (int i = 0; i < cache_associativity; i++)
            {
                if (cache_block_valid[set_index][i] == PRESENT)
                {
                    if (cache_block_tag[set_index][i] == tag)
                    {
                        // hit
                        hit_or_miss = HIT;

                        if (i < set_priority_divider[set_index])
                        { // hit in high priority group
                            cache_block_latest_access_time[set_index][i] = inst_count;
                        }
                        else
                        { // hit in low priority group => move to high priority group
                            int set_priority_divider_index = set_priority_divider[set_index];

                            int temp_block_valid = cache_block_valid[set_index][set_priority_divider_index];
                            int temp_block_tag = cache_block_tag[set_index][set_priority_divider_index];
                            int temp_block_access_time = cache_block_latest_access_time[set_index][set_priority_divider_index];
                            int temp_block_data = cache_block_data[set_index][set_priority_divider_index];

                            cache_block_valid[set_index][set_priority_divider_index] = cache_block_valid[set_index][i];
                            cache_block_tag[set_index][set_priority_divider_index] = cache_block_tag[set_index][i];
                            cache_block_latest_access_time[set_index][set_priority_divider_index] = inst_count;
                            cache_block_data[set_index][set_priority_divider_index] = cache_block_data[set_index][i];

                            cache_block_valid[set_index][i] = temp_block_valid;
                            cache_block_tag[set_index][i] = temp_block_tag;
                            cache_block_latest_access_time[set_index][i] = temp_block_access_time;
                            cache_block_data[set_index][i] = temp_block_data;

                            set_priority_divider[set_index] += 1;
                        }
                        break;
                    }
                }
                else
                {
                    if (first_non_valid_index == -1)
                    {
                        first_non_valid_index = i;
                    }
                }
            }

#if DEBUG
            cout << "first_non_valid_index: " << first_non_valid_index << endl;
#endif

            if (hit_or_miss == MISS)
            {
                if (first_non_valid_index != -1)
                {
                    // load from main memory to cache
                    cache_block_data[set_index][first_non_valid_index] = main_memory[memory_address];
                    cache_block_tag[set_index][first_non_valid_index] = tag;
                    cache_block_valid[set_index][first_non_valid_index] = PRESENT;
                    cache_block_latest_access_time[set_index][first_non_valid_index] = inst_count;
                }
                else
                {
                    // replacement: replace using LRU from low priority group, if low priority group is empty (i.e. all blocks are high priority) then use LRU in high priority group.
                    int start_index = set_priority_divider[set_index];
                    int end_index = cache_associativity - 1;
                    if (set_priority_divider[set_index] == cache_associativity)
                    { // LRU in high priority group
                        start_index = 0;
                    }

                    int least_recent_time = inst_count + 1;
                    int LRU_index = 0;
                    for (int i = start_index; i <= end_index; i++)
                    {
                        if (cache_block_latest_access_time[set_index][i] < least_recent_time)
                        {
                            least_recent_time = cache_block_latest_access_time[set_index][i];
                            LRU_index = i;
                        }
                    }

                    // load from main memory to cache
                    cache_block_data[set_index][LRU_index] = main_memory[memory_address];
                    cache_block_tag[set_index][LRU_index] = tag;
                    cache_block_valid[set_index][LRU_index] = PRESENT;
                    cache_block_latest_access_time[set_index][LRU_index] = inst_count;
                }
            }
        }
        else if (inst_type == W)
        {
        }
        else
        {
            cout << "Incorrect inst type" << endl;
            return -1;
        }

        // move lines to low priority group if not accessed for T cache accesses
        for (int i = 0; i < total_sets; i++)
        {
            for (int j = 0; j < cache_associativity; j++)
            {
                if (j < set_priority_divider[i] && inst_count - cache_block_latest_access_time[i][j] >= T)
                {
                    int set_priority_divider_index = set_priority_divider[set_index];

                    int temp_block_valid = cache_block_valid[i][set_priority_divider_index - 1];
                    int temp_block_tag = cache_block_tag[i][set_priority_divider_index - 1];
                    int temp_block_access_time = cache_block_latest_access_time[i][set_priority_divider_index - 1];
                    int temp_block_data = cache_block_data[i][set_priority_divider_index - 1];

                    cache_block_valid[i][set_priority_divider_index - 1] = cache_block_valid[i][j];
                    cache_block_tag[i][set_priority_divider_index - 1] = cache_block_tag[i][j];
                    cache_block_latest_access_time[i][set_priority_divider_index - 1] = cache_block_latest_access_time[i][j];
                    cache_block_data[i][set_priority_divider_index - 1] = cache_block_data[i][j];

                    cache_block_valid[i][j] = temp_block_valid;
                    cache_block_tag[i][j] = temp_block_tag;
                    cache_block_latest_access_time[i][j] = temp_block_access_time;
                    cache_block_data[i][j] = temp_block_data;

                    set_priority_divider[i] -= 1;
                }
            }
        }

        inst_count++;

#if DEBUG
        cout << "hit/miss: " << (hit_or_miss == HIT ? "HIT" : "MISS") << endl;
        cout << "******************** CACHE ***************************" << endl;
        for (int s = 0; s < total_sets; s++)
        {
            cout << "############################" << endl;
            cout << "Set: " << s << endl;
            cout << "set_priority_divider: " << set_priority_divider[s] << endl;

            for (int ca = 0; ca < cache_associativity; ca++)
            {
                cout << "----------------------------" << endl;
                cout << "ca: " << ca << endl;
                cout << "cache_block_valid: " << cache_block_valid[s][ca] << endl;
                cout << "cache_block_tag: " << cache_block_tag[s][ca] << endl;
                cout << "cache_block_latest_access_time: " << cache_block_latest_access_time[s][ca] << endl;

                cout << "cache_block_data: " << cache_block_data[s][ca] << endl;
            }
        }
        cout << "*****************************************************" << endl;
#endif
    }

    return 0;
}

int main(int argc, char **argv)
{

    vector<vector<int>> instructions1{
        {1, W, 1},
        {5, W, 5},
        {6, W, 6},
        {1, R},
        {9, R},
        {17, R},
        {25, R},
    };

    vector<vector<int>> instructions2{
        {1, R},
        {9, R},
        {17, R},
        {25, R},
    };

    vector<vector<int>> instructions3{
        {0, R},
        {1, R},
        {2, R},
        {3, R},
        {4, R},
        {5, R},
        {6, R},
        {16, R},
        {17, R},
    };

    cache_sim(16, 2, 2, 4, instructions3);

    return 0;
}