#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <time.h>

using namespace std;

#define MAX_MEM_ADDRESS 512
#define MAX_DATA 1024
#define R 1
#define W 0

int get_ceil(int arr[], int r, int l, int h)
{
    int mid;
    while (l < h)
    {
        mid = l + ((h - l) >> 1);
        (r > arr[mid]) ? (l = mid + 1) : (h = mid);
    }
    return (arr[l] >= r) ? l : -1;
}

int get_random_from_freq(vector<int> &arr, vector<int> &freq)
{
    int n = arr.size();
    int prefix[n], i;
    prefix[0] = freq.at(0);
    for (i = 1; i < n; ++i)
        prefix[i] = prefix[i - 1] + freq.at(i);

    int r = (rand() % prefix[n - 1]) + 1;

    int indexc = get_ceil(prefix, r, 0, n - 1);
    return arr.at(indexc);
}

void generate(string filename, int access_requests, int distinct_mem_address)
{
    ofstream inputfile;
    inputfile.open(filename);
    srand(time(0));

    // random memory addresses generated with random frequency
    vector<int> memory_address_vector;
    vector<int> memory_address_freq;
    int frequency_total = 0;

    for (int i = 0; i < distinct_mem_address; i++)
    {
        memory_address_vector.push_back(rand() % (MAX_MEM_ADDRESS + 1));
        int rand_freq = rand() % (MAX_MEM_ADDRESS + 1);
        memory_address_freq.push_back(rand_freq);
        frequency_total += rand_freq;
    }

    // read or writes generated with frequency as specified in read_or_write_freq vector
    vector<int> read_or_write_vector{R, W};
    vector<int> read_or_write_freq{1, 1};

    // generate input file
    inputfile << "16 <cache size in bytes>\n2 <cache block size in bytes>\n2 <cache associativity>\n4 <T>\n";

    int total_reads = 0;
    for (int i = 0; i < access_requests; i++)
    {
        int memory_address = get_random_from_freq(memory_address_vector, memory_address_freq);
        int read_or_write = get_random_from_freq(read_or_write_vector, read_or_write_freq);
        if (read_or_write == R)
        {
            total_reads++;
            inputfile << memory_address << ", R" << endl;
        }
        else
        {
            inputfile << memory_address << ", W, " << rand() % (MAX_DATA + 1) << endl;
        }
    }

    cout << "Input file " << filename << " "
         << "generated with: " << endl;
    cout << "Number of memory access requests: " << access_requests << endl;
    cout << "Read requests: " << total_reads << "   Write requests: " << access_requests - total_reads << endl;
    cout << "Memory addresses & their freqeuncy in access requests: " << endl;
    for (int i = 0; i < memory_address_vector.size(); i++)
    {
        cout << i + 1 << ") memory address: " << memory_address_vector.at(i) << "      frequency: " << (float)memory_address_freq.at(i) * 100 / frequency_total << " %" << endl;
    }

    inputfile.close();
}

int main()
{
    string filename = "input/graph/inp_graph.txt";
    int access_requests = 20000; // number of cache access requests to generate
    int distinct_mem_address = 20; // number of distinct memory addresses in requests
    generate(filename, access_requests, distinct_mem_address);

    return 0;
}