#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <time.h>

using namespace std;

#define ACCESS_REQUESTS 100
#define MAX_MEM_ADDRESS 512
#define MAX_DATA 1024
#define DISTINCT_MEM_ADDRESS 6
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

int main()
{
    ofstream inputfile;
    inputfile.open("input/inp_gen6.txt");
    srand(time(0));

    vector<int> memory_address_vector;
    vector<int> frequency;

    for (int i = 0; i < DISTINCT_MEM_ADDRESS; i++)
    {
        memory_address_vector.push_back(rand() % (MAX_MEM_ADDRESS + 1));
        frequency.push_back(rand() % ((MAX_MEM_ADDRESS / 4) + 1));
    }

    int frequency_total = 0;
    for (auto i = frequency.begin(); i != frequency.end(); i++)
    {
        frequency_total += *i;
    }

    // generate input file
    inputfile << "16 <cache size in bytes>\n2 <cache block size in bytes>\n2 <cache associativity>\n4 <T>\n";

    int total_reads = 0;
    for (int i = 0; i < ACCESS_REQUESTS; i++)
    {
        int memory_address = get_random_from_freq(memory_address_vector, frequency);
        int read_or_write = rand() % 2;
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

    cout << "Input generated with: " << endl;
    cout << "Number of memory access requests: " << ACCESS_REQUESTS << endl;
    cout << "Read requests: " << total_reads << "   Write requests: " << ACCESS_REQUESTS - total_reads << endl;
    cout << "Memory addresses & their freqeuncy in access requests: " << endl;
    for (int i = 0; i < memory_address_vector.size(); i++)
    {
        cout << i+1 << ") memory address: " << memory_address_vector.at(i) << "      frequency: " << (float)frequency.at(i) * 100 / frequency_total << " %" << endl;
    }

    inputfile.close();
}