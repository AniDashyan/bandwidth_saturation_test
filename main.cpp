#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <iomanip>
#ifdef __AVX2__
#include <immintrin.h>
#endif

using namespace std::chrono;

const size_t BUFFER_SIZE = 256ULL * 1024ULL * 1024ULL; // 256 MB for testing (adjustable to 1ULL * 1024 * 1024 * 1024 for 1 GB)
const size_t NUM_THREADS = std::thread::hardware_concurrency();
const int64_t NUM_ITERATIONS = 1000;

std::atomic<size_t> totalBytesProcessed(0);

alignas(64) std::vector<int64_t> buffer(BUFFER_SIZE / sizeof(int64_t));

void threadWorker(int threadId, size_t chunkSize) {
    size_t bytesProcessed = 0;
    size_t startIdx = threadId * chunkSize;
    size_t endIdx = std::min(startIdx + chunkSize, buffer.size());
    int64_t* buf = buffer.data();

    if (startIdx >= BUFFER_SIZE) {
        std::cout << "Thread " << threadId << " skipped: startIdx exceeds buffer" << std::endl;
        return;
    }

#ifdef __AVX2__
    __m256i pattern = _mm256_set1_epi64x(0xDEADBEEFDEADBEEF);

    for (int64_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
        size_t i = startIdx;
        for (; i <= endIdx - 16; i += 16) {
            _mm256_stream_si256(reinterpret_cast<__m256i*>(&buf[i]), pattern);
            _mm256_stream_si256(reinterpret_cast<__m256i*>(&buf[i + 4]), pattern);
            _mm256_stream_si256(reinterpret_cast<__m256i*>(&buf[i + 8]), pattern);
            _mm256_stream_si256(reinterpret_cast<__m256i*>(&buf[i + 12]), pattern);
            bytesProcessed += 128;
        }
        for (; i < endIdx; ++i) {
            buf[i] = 42;
            bytesProcessed += sizeof(int64_t);
        }
    }
#else
    for (int64_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
        for (size_t i = startIdx; i < endIdx; ++i) {
            buf[i] = 42;
            bytesProcessed += sizeof(int64_t);
        }
    }
#endif

    totalBytesProcessed.fetch_add(bytesProcessed, std::memory_order_relaxed);
}

int main() {
    size_t chunkSize = buffer.size() / NUM_THREADS;
    chunkSize -= chunkSize % 16;

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    auto startTime = steady_clock::now();

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(threadWorker, i, chunkSize);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto endTime = steady_clock::now();
    auto totalDuration = duration_cast<milliseconds>(endTime - startTime);

    double totalGB = totalBytesProcessed / (1024.0 * 1024.0 * 1024.0);
    double throughput = totalGB / (totalDuration.count() / 1000.0);


    // Top border
    std::cout << "+" << std::setw(60) << std::setfill('-') << "" << "+" << std::setfill(' ') << std::endl;

    // Header
    std::cout << "|" << std::left << std::setw(30) << " Label" << "|" 
              << std::right  << "Value" << std::setw(25) << "|" << std::endl;

    // Separator
    std::cout << "+" << std::setw(60) << std::setfill('-') << "" << "+" << std::setfill(' ') << std::endl;

    // Rows
    std::cout << "|" << std::left << std::setw(30) << " Number of threads" << "|" 
              << std::right << NUM_THREADS << std::setw(28) << "|" << std::endl;

    std::cout << "|" << std::left << std::setw(30) << " Total time" << "|" 
              << std::right << (std::to_string(totalDuration.count()) + " ms") << std::setw(23) << "|" << std::endl;

    std::cout << "|" << std::left << std::setw(30) << " Total data processed" << "|" 
              << std::right << std::fixed << std::setprecision(3) << (totalGB) << " GB"  << std::setw(20) << "|" << std::endl;

    std::cout << "|" << std::left << std::setw(30) << " Throughput" << "|" 
              << std::right << std::fixed << std::setprecision(3) << (throughput) << " GB/s" << std::setw(19) << "|" << std::endl;

    std::cout << "|" << std::left << std::setw(30) << " Memory Bandwidth Utilization"  << "|" 
              << std::right << std::setprecision(3) << ((throughput / 50.0) * 100) << " % (assuming 50 GB/s)" << std::setw(2) << "|" << std::endl;

    // Bottom border
    std::cout << "+" << std::setw(60) << std::setfill('-') << "" << "+" << std::setfill(' ') << std::endl;

    return (0);
}
