#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <atomic>
#include <iomanip>
#ifdef __AVX2__
#include <immintrin.h>
#endif

using namespace std::chrono;

const size_t BUFFER_SIZE = 1ULL * 1024ULL * 1024ULL * 1024ULL; // 1 GB
const size_t NUM_THREADS = std::thread::hardware_concurrency();
const int64_t NUM_ITERATIONS = 1000;
const size_t STRIDE = 16; // 128 bytes stride (16 * 8 bytes)

std::atomic<size_t> totalBytesProcessed(0);
alignas(64) std::vector<int64_t> buffer(BUFFER_SIZE / sizeof(int64_t));

void threadWorker(int threadId, size_t chunkSize) {
    size_t bytesProcessed = 0;
    size_t startIdx = threadId * chunkSize;
    size_t endIdx = std::min(startIdx + chunkSize, buffer.size());
    int64_t* buf = buffer.data();

    if (startIdx >= buffer.size()) {
        std::cout << "Thread " << threadId << " skipped: startIdx exceeds buffer size" << std::endl;
        return;
    }

#ifdef __AVX2__
    __m256i pattern = _mm256_set1_epi64x(0xDEADBEEFDEADBEEF);

    for (int64_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
        size_t i = startIdx;
        for (; i + 4 * STRIDE <= endIdx; i += STRIDE) {
            __m256i data = _mm256_load_si256(reinterpret_cast<const __m256i*>(&buf[i]));
            _mm256_stream_si256(reinterpret_cast<__m256i*>(&buf[i]), pattern);
            bytesProcessed += 64; // 32 bytes read + 32 bytes write
        }
        for (; i < endIdx; i += STRIDE) {
            int64_t value = buf[i];
            buf[i] = 42;
            bytesProcessed += 2 * sizeof(int64_t);
        }
    }
#else
    for (int64_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
        for (size_t i = startIdx; i < endIdx; i += STRIDE) {
            int64_t value = buf[i];
            buf[i] = 42;
            bytesProcessed += 2 * sizeof(int64_t);
        }
    }
#endif

    totalBytesProcessed.fetch_add(bytesProcessed, std::memory_order_relaxed);
}

int main() {
    std::cout << "Hardware concurrency: " << NUM_THREADS << std::endl;

    size_t chunkSize = buffer.size() / NUM_THREADS;
    chunkSize -= chunkSize % STRIDE;

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

    std::cout << "+" << std::setw(60) << std::setfill('-') << "" << "+" << std::setfill(' ') << std::endl;
    std::cout << "|" << std::left << std::setw(30) << " Label" << "|" 
              << std::right << "Value" << std::setw(25) << "|" << std::endl;
    std::cout << "+" << std::setw(60) << std::setfill('-') << "" << "+" << std::setfill(' ') << std::endl;

    std::cout << "|" << std::left << std::setw(30) << " Number of threads" << "|" 
              << std::right << NUM_THREADS << std::setw(28) << "|" << std::endl;
    std::cout << "|" << std::left << std::setw(30) << " Total time" << std::setw(3) << "|" 
              << std::right << totalDuration.count() << " ms" << std::setw(20) << "|" << std::endl;
    std::cout << "|" << std::left << std::setw(30) << " Total data processed" << "|" 
              << std::right << std::fixed << std::setprecision(3) << totalGB << " GB" << std::setw(20) << "|" << std::endl;
    std::cout << "|" << std::left << std::setw(30) << " Throughput" << "|" 
              << std::right << std::fixed << std::setprecision(3) << throughput << " GB/s" << std::setw(19) << "|" << std::endl;
    std::cout << "|" << std::left << std::setw(30) << " Memory Bandwidth Utilization" << "|" 
              << std::right << std::setprecision(3) << (throughput / 50.0 * 100) << " % (assuming 50 GB/s)" << std::setw(3) << " |" << std::endl;
    std::cout << "+" << std::setw(60) << std::setfill('-') << "" << "+" << std::setfill(' ') << std::endl;

    return 0;
}