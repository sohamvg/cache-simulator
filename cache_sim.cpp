#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <math.h>

using namespace std;

#define MAIN_MEMORY_BLOCKS 2048
#define R 1
#define W 0
#define PRESENT 1
#define NOT_PRESENT 0
#define HIT 1
#define MISS 0
#define DEBUG 1

// check validity of number (should be exponent of 2)
bool check_validity(int n)
{
    if (n == 0)
        return false;

    return ceil(log2(n)) == floor(log2(n));
}

// simulate cache
int cache_sim(int cache_size, int cache_block_size, int cache_associativity, int T, vector<vector<int>> &instructions)
{
    if (!(check_validity(cache_size) && check_validity(cache_block_size) && check_validity(cache_associativity)))
    {
        cout << "Invalid parameters" << endl;
        exit(EXIT_FAILURE);
    }

    int total_cache_blocks = cache_size / cache_block_size;
    int total_sets = total_cache_blocks / cache_associativity;

    if (total_cache_blocks < 1 || total_sets < 1) {
        cout << "Invalid parameters" << endl;
        exit(EXIT_FAILURE);
    }

    int main_memory[MAIN_MEMORY_BLOCKS]; // block addressable (block size same as cache_block_size)
    int test_memory[MAIN_MEMORY_BLOCKS];

    // initializing memory
    for (int i = 0; i < MAIN_MEMORY_BLOCKS; i++)
    {
        main_memory[i] = i;
        test_memory[i] = i;
    }

    int cache_block_data[total_sets][cache_associativity] = {0}; // block addressable
    int cache_block_valid[total_sets][cache_associativity] = {NOT_PRESENT};
    int cache_block_tag[total_sets][cache_associativity] = {0};
    int cache_block_dirty[total_sets][cache_associativity] = {0};
    int cache_block_latest_access_time[total_sets][cache_associativity] = {0};
    int set_priority_divider[total_sets] = {0}; // = total lines in HIGH PRIORITY group = index of first line in LOW PRIORITY group

#if DEBUG
    std::cout << "cache_size: " << cache_size << endl;
    std::cout << "cache_block_size: " << cache_block_size << endl;
    std::cout << "cache_associativity: " << cache_associativity << endl;
    std::cout << "T: " << T << endl;
    std::cout << "total_cache_blocks: " << total_cache_blocks << endl;
    std::cout << "total_sets: " << total_sets << endl;

    std::cout << "******************** CACHE ***************************" << endl;
    for (int s = 0; s < total_sets; s++)
    {
        std::cout << "############################" << endl;
        std::cout << "Set: " << s << endl;
        std::cout << "set_priority_divider: " << set_priority_divider[s] << endl;

        for (int ca = 0; ca < cache_associativity; ca++)
        {
            std::cout << "----------------------------" << endl;
            std::cout << "ca: " << ca << endl;
            std::cout << "cache_block_valid: " << cache_block_valid[s][ca] << endl;
            std::cout << "cache_block_tag: " << cache_block_tag[s][ca] << endl;
            std::cout << "cache_block_latest_access_time: " << cache_block_latest_access_time[s][ca] << endl;
            std::cout << "cache_block_dirty: " << cache_block_dirty[s][ca] << endl;
            std::cout << "cache_block_data: " << cache_block_data[s][ca] << endl;
        }
    }
    std::cout << "*****************************************************" << endl;

#endif

    int inst_count = 0;
    int total_reads = 0;
    int total_read_hits = 0;
    int total_writes = 0;
    int total_write_hits = 0;

    for (auto it = instructions.begin(); it != instructions.end(); it++)
    {
        vector<int> inst = *it;
        if (inst.size() < 2)
        {
            std::cout << "Instruction error" << endl;
            exit(EXIT_FAILURE);
        }
        int memory_address = inst.at(0); // this is block address
        int inst_type = inst.at(1);

        /*
            byte address:   <tag><set_index><offset>
            block address:  <tag><set_index>
        */

        int set_index = memory_address % total_sets;
        int tag = memory_address / total_sets;

#if DEBUG
        std::cout << "=========================== instruction ==============================" << endl;
        std::cout << "inst_count: " << inst_count << endl;
        std::cout << "memory_address: " << memory_address << endl;
        std::cout << "inst_type: " << (inst_type == R ? "R" : "W") << endl;
        std::cout << "set_index: " << set_index << endl;
        std::cout << "tag: " << tag << endl;
#endif

        int hit_or_miss = MISS;

        if (inst_type == R)
        {
            total_reads++;
            int read_result = 0;
            int first_non_valid_index = -1;
            for (int i = 0; i < cache_associativity; i++)
            {
                if (cache_block_valid[set_index][i] == PRESENT)
                {
                    if (cache_block_tag[set_index][i] == tag)
                    {
                        // hit
                        hit_or_miss = HIT;
                        total_read_hits++;
                        read_result = cache_block_data[set_index][i];

                        if (i < set_priority_divider[set_index])
                        {
                            // hit in high priority group
                            cache_block_latest_access_time[set_index][i] = inst_count; // update accessed block time
                        }
                        else
                        {
                            // hit in low priority group => move to high priority group
                            int set_priority_divider_index = set_priority_divider[set_index];

                            int temp_block_valid = cache_block_valid[set_index][set_priority_divider_index];
                            int temp_block_tag = cache_block_tag[set_index][set_priority_divider_index];
                            int temp_block_dirty = cache_block_dirty[set_index][set_priority_divider_index];
                            int temp_block_access_time = cache_block_latest_access_time[set_index][set_priority_divider_index];
                            int temp_block_data = cache_block_data[set_index][set_priority_divider_index];

                            cache_block_valid[set_index][set_priority_divider_index] = cache_block_valid[set_index][i];
                            cache_block_tag[set_index][set_priority_divider_index] = cache_block_tag[set_index][i];
                            cache_block_dirty[set_index][set_priority_divider_index] = cache_block_dirty[set_index][i];
                            cache_block_latest_access_time[set_index][set_priority_divider_index] = cache_block_latest_access_time[set_index][i];
                            cache_block_data[set_index][set_priority_divider_index] = cache_block_data[set_index][i];

                            cache_block_valid[set_index][i] = temp_block_valid;
                            cache_block_tag[set_index][i] = temp_block_tag;
                            cache_block_dirty[set_index][i] = temp_block_dirty;
                            cache_block_latest_access_time[set_index][i] = temp_block_access_time;
                            cache_block_data[set_index][i] = temp_block_data;

                            cache_block_latest_access_time[set_index][set_priority_divider_index] = inst_count; // update accessed block time

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
            std::cout << "first_non_valid_index: " << first_non_valid_index << endl;
#endif

            if (hit_or_miss == MISS)
            {
                if (first_non_valid_index != -1)
                {
                    // load from main memory to cache
                    cache_block_data[set_index][first_non_valid_index] = main_memory[memory_address];
                    cache_block_tag[set_index][first_non_valid_index] = tag;
                    cache_block_dirty[set_index][first_non_valid_index] = 0;
                    cache_block_valid[set_index][first_non_valid_index] = PRESENT;
                    cache_block_latest_access_time[set_index][first_non_valid_index] = inst_count;
                }
                else
                {
                    // replacement: replace using LRU from low priority group, if low priority group is empty (i.e. all blocks are high priority) then use LRU in high priority group.

                    if (set_priority_divider[set_index] < cache_associativity)
                    {
                        // LRU in low priority group
                        int start_index = set_priority_divider[set_index];
                        int least_recent_time = inst_count + 1;
                        int LRU_index = 0;
                        for (int i = start_index; i < cache_associativity; i++)
                        {
                            if (cache_block_latest_access_time[set_index][i] < least_recent_time)
                            {
                                least_recent_time = cache_block_latest_access_time[set_index][i];
                                LRU_index = i;
                            }
                        }

                        if (cache_block_dirty[set_index][LRU_index] == 1)
                        {
                            // when replacing dirty block, write back to main memory
                            int corresponding_main_memory_address = (cache_block_tag[set_index][LRU_index] * total_sets) + set_index;
                            main_memory[corresponding_main_memory_address] = cache_block_data[set_index][LRU_index];
                        }

                        // load from main memory to cache
                        cache_block_data[set_index][LRU_index] = main_memory[memory_address];
                        cache_block_tag[set_index][LRU_index] = tag;
                        cache_block_dirty[set_index][LRU_index] = 0;
                        cache_block_valid[set_index][LRU_index] = PRESENT;
                        cache_block_latest_access_time[set_index][LRU_index] = inst_count;
                    }
                    else
                    {
                        // LRU in high priority group
                        int start_index = 0;
                        int least_recent_time = inst_count + 1;
                        int LRU_index = 0;
                        for (int i = start_index; i < cache_associativity; i++)
                        {
                            if (cache_block_latest_access_time[set_index][i] < least_recent_time)
                            {
                                least_recent_time = cache_block_latest_access_time[set_index][i];
                                LRU_index = i;
                            }
                        }

                        if (cache_block_dirty[set_index][LRU_index] == 1)
                        {
                            // when replacing dirty block, write back to main memory
                            int corresponding_main_memory_address = (cache_block_tag[set_index][LRU_index] * total_sets) + set_index;
                            main_memory[corresponding_main_memory_address] = cache_block_data[set_index][LRU_index];
                        }

                        set_priority_divider[set_index]--;
                        int set_priority_divider_index = set_priority_divider[set_index];

                        cache_block_data[set_index][LRU_index] = cache_block_data[set_index][set_priority_divider_index];
                        cache_block_tag[set_index][LRU_index] = cache_block_tag[set_index][set_priority_divider_index];
                        cache_block_dirty[set_index][LRU_index] = cache_block_dirty[set_index][set_priority_divider_index];
                        cache_block_valid[set_index][LRU_index] = cache_block_valid[set_index][set_priority_divider_index];
                        cache_block_latest_access_time[set_index][LRU_index] = cache_block_latest_access_time[set_index][set_priority_divider_index];

                        // load from main memory to cache
                        cache_block_data[set_index][set_priority_divider_index] = main_memory[memory_address];
                        cache_block_tag[set_index][set_priority_divider_index] = tag;
                        cache_block_dirty[set_index][set_priority_divider_index] = 0;
                        cache_block_valid[set_index][set_priority_divider_index] = PRESENT;
                        cache_block_latest_access_time[set_index][set_priority_divider_index] = inst_count;
                    }
                }

                read_result = main_memory[memory_address];
            }

#if DEBUG
            cout << "Read result: " << read_result << endl;
            cout << "Read result from test_memory: " << test_memory[memory_address] << endl;
            if (read_result == test_memory[memory_address])
            {
                cout << "Correct read!!" << endl;
            }
            else
            {
                cout << "Wrong read!!" << endl;
            }
#endif
        }
        else if (inst_type == W)
        {
            // write back with write allocate

            if (inst.size() != 3)
            {
                std::cout << "Instruction error" << endl;
                exit(EXIT_FAILURE);
            }
            int write_data = inst.at(2);

            test_memory[memory_address] = write_data;
            total_writes++;

            int first_non_valid_index = -1;
            for (int i = 0; i < cache_associativity; i++)
            {
                if (cache_block_valid[set_index][i] == PRESENT)
                {
                    if (cache_block_tag[set_index][i] == tag)
                    {
                        // hit
                        hit_or_miss = HIT;
                        total_write_hits++;

                        if (i < set_priority_divider[set_index])
                        {
                            // hit in high priority group
                            cache_block_data[set_index][i] = write_data;
                            cache_block_dirty[set_index][i] = 1;
                            cache_block_latest_access_time[set_index][i] = inst_count; // update accessed block time
                        }
                        else
                        {
                            // hit in low priority group => move to high priority group
                            int set_priority_divider_index = set_priority_divider[set_index];

                            int temp_block_valid = cache_block_valid[set_index][set_priority_divider_index];
                            int temp_block_tag = cache_block_tag[set_index][set_priority_divider_index];
                            int temp_block_dirty = cache_block_dirty[set_index][set_priority_divider_index];
                            int temp_block_access_time = cache_block_latest_access_time[set_index][set_priority_divider_index];
                            int temp_block_data = cache_block_data[set_index][set_priority_divider_index];

                            cache_block_valid[set_index][set_priority_divider_index] = cache_block_valid[set_index][i];
                            cache_block_tag[set_index][set_priority_divider_index] = cache_block_tag[set_index][i];
                            cache_block_dirty[set_index][set_priority_divider_index] = cache_block_dirty[set_index][i];
                            cache_block_latest_access_time[set_index][set_priority_divider_index] = cache_block_latest_access_time[set_index][i];
                            cache_block_data[set_index][set_priority_divider_index] = cache_block_data[set_index][i];

                            cache_block_valid[set_index][i] = temp_block_valid;
                            cache_block_tag[set_index][i] = temp_block_tag;
                            cache_block_dirty[set_index][i] = temp_block_dirty;
                            cache_block_latest_access_time[set_index][i] = temp_block_access_time;
                            cache_block_data[set_index][i] = temp_block_data;

                            cache_block_data[set_index][set_priority_divider_index] = write_data;
                            cache_block_dirty[set_index][set_priority_divider_index] = 1;
                            cache_block_latest_access_time[set_index][set_priority_divider_index] = inst_count; // update accessed block time

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
            std::cout << "first_non_valid_index: " << first_non_valid_index << endl;
#endif

            if (hit_or_miss == MISS)
            {
                // write to main memory
                main_memory[memory_address] = write_data;

                if (first_non_valid_index != -1)
                {
                    // load from main memory to cache
                    cache_block_data[set_index][first_non_valid_index] = main_memory[memory_address];
                    cache_block_tag[set_index][first_non_valid_index] = tag;
                    cache_block_valid[set_index][first_non_valid_index] = PRESENT;
                    cache_block_dirty[set_index][first_non_valid_index] = 0;
                    cache_block_latest_access_time[set_index][first_non_valid_index] = inst_count;
                }
                else
                {
                    // replacement: replace using LRU from low priority group, if low priority group is empty (i.e. all blocks are high priority) then use LRU in high priority group.

                    if (set_priority_divider[set_index] < cache_associativity)
                    {
                        // LRU in low priority group
                        int start_index = set_priority_divider[set_index];
                        int least_recent_time = inst_count + 1;
                        int LRU_index = 0;
                        for (int i = start_index; i < cache_associativity; i++)
                        {
                            if (cache_block_latest_access_time[set_index][i] < least_recent_time)
                            {
                                least_recent_time = cache_block_latest_access_time[set_index][i];
                                LRU_index = i;
                            }
                        }

                        if (cache_block_dirty[set_index][LRU_index] == 1)
                        {
                            // when replacing dirty block, write back to main memory
                            int corresponding_main_memory_address = (cache_block_tag[set_index][LRU_index] * total_sets) + set_index;
                            main_memory[corresponding_main_memory_address] = cache_block_data[set_index][LRU_index];
                        }

                        // load from main memory to cache
                        cache_block_data[set_index][LRU_index] = main_memory[memory_address];
                        cache_block_tag[set_index][LRU_index] = tag;
                        cache_block_valid[set_index][LRU_index] = PRESENT;
                        cache_block_dirty[set_index][LRU_index] = 0;
                        cache_block_latest_access_time[set_index][LRU_index] = inst_count;
                    }
                    else
                    {
                        // LRU in high priority group
                        int start_index = 0;
                        int least_recent_time = inst_count + 1;
                        int LRU_index = 0;
                        for (int i = start_index; i < cache_associativity; i++)
                        {
                            if (cache_block_latest_access_time[set_index][i] < least_recent_time)
                            {
                                least_recent_time = cache_block_latest_access_time[set_index][i];
                                LRU_index = i;
                            }
                        }

                        if (cache_block_dirty[set_index][LRU_index] == 1)
                        {
                            // when replacing dirty block, write back to main memory
                            int corresponding_main_memory_address = (cache_block_tag[set_index][LRU_index] * total_sets) + set_index;
                            main_memory[corresponding_main_memory_address] = cache_block_data[set_index][LRU_index];
                        }

                        set_priority_divider[set_index]--;
                        int set_priority_divider_index = set_priority_divider[set_index];

                        cache_block_data[set_index][LRU_index] = cache_block_data[set_index][set_priority_divider_index];
                        cache_block_tag[set_index][LRU_index] = cache_block_tag[set_index][set_priority_divider_index];
                        cache_block_dirty[set_index][LRU_index] = cache_block_dirty[set_index][set_priority_divider_index];
                        cache_block_valid[set_index][LRU_index] = cache_block_valid[set_index][set_priority_divider_index];
                        cache_block_latest_access_time[set_index][LRU_index] = cache_block_latest_access_time[set_index][set_priority_divider_index];

                        // load from main memory to cache
                        cache_block_data[set_index][set_priority_divider_index] = main_memory[memory_address];
                        cache_block_tag[set_index][set_priority_divider_index] = tag;
                        cache_block_dirty[set_index][set_priority_divider_index] = 0;
                        cache_block_valid[set_index][set_priority_divider_index] = PRESENT;
                        cache_block_latest_access_time[set_index][set_priority_divider_index] = inst_count;
                    }
                }
            }
        }
        else
        {
            std::cout << "Incorrect inst type" << endl;
            return -1;
        }

        // move lines to low priority group if not accessed for T cache accesses
        for (int i = 0; i < total_sets; i++)
        {
            for (int j = 0; j < cache_associativity; j++)
            {
                if (j < set_priority_divider[i] && inst_count - cache_block_latest_access_time[i][j] >= T)
                {
#if DEBUG
                    std::cout << "moving line to low priority group..." << endl;
                    std::cout << "set: " << i << endl;
                    std::cout << "ca: " << j << endl;
#endif
                    int set_priority_divider_index = set_priority_divider[i];

                    int temp_block_valid = cache_block_valid[i][set_priority_divider_index - 1];
                    int temp_block_tag = cache_block_tag[i][set_priority_divider_index - 1];
                    int temp_block_dirty = cache_block_dirty[i][set_priority_divider_index - 1];
                    int temp_block_access_time = cache_block_latest_access_time[i][set_priority_divider_index - 1];
                    int temp_block_data = cache_block_data[i][set_priority_divider_index - 1];

                    cache_block_valid[i][set_priority_divider_index - 1] = cache_block_valid[i][j];
                    cache_block_tag[i][set_priority_divider_index - 1] = cache_block_tag[i][j];
                    cache_block_dirty[i][set_priority_divider_index - 1] = cache_block_dirty[i][j];
                    cache_block_latest_access_time[i][set_priority_divider_index - 1] = cache_block_latest_access_time[i][j];
                    cache_block_data[i][set_priority_divider_index - 1] = cache_block_data[i][j];

                    cache_block_valid[i][j] = temp_block_valid;
                    cache_block_tag[i][j] = temp_block_tag;
                    cache_block_dirty[i][j] = temp_block_dirty;
                    cache_block_latest_access_time[i][j] = temp_block_access_time;
                    cache_block_data[i][j] = temp_block_data;

                    set_priority_divider[i] -= 1;
                }
            }
        }

        inst_count++;

#if DEBUG
        std::cout << "hit/miss: " << (hit_or_miss == HIT ? "HIT" : "MISS") << endl;
        std::cout << "******************** CACHE ***************************" << endl;
        for (int s = 0; s < total_sets; s++)
        {
            std::cout << "############################" << endl;
            std::cout << "Set: " << s << endl;
            std::cout << "set_priority_divider: " << set_priority_divider[s] << endl;

            for (int ca = 0; ca < cache_associativity; ca++)
            {
                std::cout << "----------------------------" << endl;
                std::cout << "ca: " << ca << endl;
                std::cout << "cache_block_valid: " << cache_block_valid[s][ca] << endl;
                std::cout << "cache_block_tag: " << cache_block_tag[s][ca] << endl;
                std::cout << "cache_block_latest_access_time: " << cache_block_latest_access_time[s][ca] << endl;
                std::cout << "cache_block_dirty: " << cache_block_dirty[s][ca] << endl;
                std::cout << "cache_block_data: " << cache_block_data[s][ca] << endl;
            }
        }
        std::cout << "*****************************************************" << endl;
#endif
    }

    std::cout << "******************** CACHE ***************************" << endl;
    std::cout << "#Data, Tag, Valid-status(valid=1), dirty-status(dirty=1)" << endl;
    for (int s = 0; s < total_sets; s++)
    {
        for (int ca = 0; ca < cache_associativity; ca++)
        {
            std::cout << cache_block_data[s][ca] << ", " << cache_block_tag[s][ca] << ", " << cache_block_valid[s][ca] << ", " << cache_block_dirty[s][ca] << endl;
        }
    }
    std::cout << "*****************************************************" << endl;
    std::cout << "Cache statistics: " << endl;
    std::cout << "Number of Accesses: " << inst_count << endl;
    std::cout << "Number of Reads: " << total_reads << endl;
    std::cout << "Number of Read Hits: " << total_read_hits << endl;
    std::cout << "Number of Read Misses: " << total_reads - total_read_hits << endl;
    std::cout << "Number of Writes: " << total_writes << endl;
    std::cout << "Number of Write Hits: " << total_write_hits << endl;
    std::cout << "Number of Write Misses: " << total_writes - total_write_hits << endl;
    std::cout << "Hit ratio: " << (float)(total_read_hits + total_write_hits) / inst_count << endl;

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Enter all arguments" << endl;
        exit(EXIT_SUCCESS);
    }

    ifstream infile(argv[1]);

    int cache_size, cache_block_size, cache_associativity, T;
    vector<vector<int>> instructions;

    string line;
    int line_count = 0;
    while (getline(infile, line))
    {
        if (line.size() > 0 && line[0] != '#')
        {
            line_count++;
            istringstream iss(line);

            if (line_count == 1)
            {
                iss >> cache_size;
            }
            else if (line_count == 2)
            {
                iss >> cache_block_size;
            }
            else if (line_count == 3)
            {
                iss >> cache_associativity;
            }
            else if (line_count == 4)
            {
                iss >> T;
            }
            else
            {
                string memory_address, read_or_write, write_data;
                vector<int> inst;

                if (getline(iss, memory_address, ','))
                {
                    inst.push_back(stoi(memory_address));
                }

                if (getline(iss, read_or_write, ','))
                {
                    read_or_write.erase(remove(read_or_write.begin(), read_or_write.end(), ' '), read_or_write.end());
                    if (read_or_write == "R")
                    {
                        inst.push_back(R);
                    }
                    else if (read_or_write == "W")
                    {
                        inst.push_back(W);
                    }
                    else
                    {
                        cout << "Invalid input" << endl;
                        exit(EXIT_FAILURE);
                    }
                }

                if (getline(iss, write_data, ','))
                {
                    inst.push_back(stoi(write_data));
                }
                instructions.push_back(inst);
            }
        }
    }

    cache_sim(cache_size, cache_block_size, cache_associativity, T, instructions);

    return 0;
}