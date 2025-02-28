#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#ifdef __AVX2__
#include <immintrin.h>
#endif

using namespace std::chrono;

const size_t BUFFER_SIZE = 1ULL * 1024ULL * 1024ULL * 1024ULL; // 1 GB
const size_t NUM_THREADS = std::thread::hardware_concurrency();
const int NUM_ITERATIONS = 20;

std::atomic<size_t> totalBytesProcessed(0);

alignas(64) std::vector<int64_t> buffer(BUFFER_SIZE / sizeof(int64_t), 0);

void threadWorker(int threadId, size_t chunkSize) {
    size_t bytesProcessed = 0;
    size_t startIdx = threadId * chunkSize;
    size_t endIdx = std::min(startIdx + chunkSize, buffer.size());

    int64_t* buf = buffer.data();

#ifdef __AVX2__
    __m256i pattern = _mm256_set1_epi64x(0xDEADBEEF);
    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        size_t i = startIdx;
        for (; i <= endIdx - 16; i += 16) { // Process 128 bytes per loop (4x 32-byte AVX2)
            __m256i data1 = _mm256_load_si256(reinterpret_cast<__m256i*>(&buf[i]));
            __m256i data2 = _mm256_load_si256(reinterpret_cast<__m256i*>(&buf[i + 4]));
            __m256i data3 = _mm256_load_si256(reinterpret_cast<__m256i*>(&buf[i + 8]));
            __m256i data4 = _mm256_load_si256(reinterpret_cast<__m256i*>(&buf[i + 12]));
            _mm256_stream_si256(reinterpret_cast<__m256i*>(&buf[i]), pattern);
            _mm256_stream_si256(reinterpret_cast<__m256i*>(&buf[i + 4]), data1);
            _mm256_stream_si256(reinterpret_cast<__m256i*>(&buf[i + 8]), data2);
            _mm256_stream_si256(reinterpret_cast<__m256i*>(&buf[i + 12]), data3);
            bytesProcessed += 128;
        }
        // Handle remaining elements
        for (; i < endIdx; ++i) {
            buf[i] = 0xDEADBEEF;
            bytesProcessed += sizeof(int64_t);
        }
    }
#else
    // Fallback for non-AVX2 systems
    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        for (size_t i = startIdx; i < endIdx; ++i) {
            buf[i] = 0xDEADBEEF;
            bytesProcessed += sizeof(int64_t);
        }
    }
#endif

    totalBytesProcessed += bytesProcessed;
}

int main() {
    size_t chunkSize = buffer.size() / NUM_THREADS;
    chunkSize -= chunkSize % 16; // Align to 128-byte boundary for AVX2

    std::vector<std::thread> threads;

    auto startTime = steady_clock::now();

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(threadWorker, i, chunkSize);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto endTime = steady_clock::now();
    auto totalDuration = duration_cast<milliseconds>(endTime - startTime);

    double totalMB = totalBytesProcessed / (1024.0 * 1024.0);
    double throughput = totalMB / (totalDuration.count() / 1000.0);

    std::cout << "\nTotal Results:" << std::endl;
    std::cout << "Number of threads: " << NUM_THREADS << std::endl;
    std::cout << "Total time: " << totalDuration.count() << " ms" << std::endl;
    std::cout << "Total data processed: " << totalMB << " MB" << std::endl;
    std::cout << "Throughput: " << throughput << " MB/s" << std::endl;
    std::cout << "Memory Bandwidth Utilization: " << (throughput / (50 * 1024)) * 100 << " % (assuming 50 GB/s)" << std::endl;

    return 0;
}