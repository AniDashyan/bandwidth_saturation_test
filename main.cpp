#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

using namespace std::chrono;

const size_t BUFFER_SIZE = 1 * 1024 * 1024 * 1024;
const size_t NUM_THREADS = std::thread::hardware_concurrency();
const int NUM_ITERATIONS = 10;

std::atomic<size_t> totalBytesProcessed(0); // Thread-safe counter
std::vector<double> threadDurations(NUM_THREADS, 0.0); // Store thread-specific durations

// Shared buffer
std::vector<char> buffer(BUFFER_SIZE);

void threadWorker(int threadId, size_t chunkSize) {
    size_t bytesProcessed = 0;
    auto start = steady_clock::now();
    
    // Each thread works on its own portion of the buffer
    size_t startIdx = threadId * chunkSize;
    size_t endIdx = std::min(startIdx + chunkSize, BUFFER_SIZE);

    // Read and write operations
    for (size_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
        for (size_t i = startIdx; i < endIdx; ++i) {
            buffer[i] = static_cast<char>(buffer[i] + 1);
            bytesProcessed++;
        }
    }

    auto end = steady_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    threadDurations[threadId] = duration.count();
    totalBytesProcessed += bytesProcessed; // Alternative to fetch_add
}

int main() {
    size_t chunkSize = BUFFER_SIZE / NUM_THREADS;
    std::vector<std::thread> threads;

    auto startTime = steady_clock::now();

    // Create threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(threadWorker, i, chunkSize);
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    auto endTime = steady_clock::now();
    auto totalDuration = duration_cast<milliseconds>(endTime - startTime);

    // Display thread-specific results
    for (int i = 0; i < NUM_THREADS; ++i) {
        std::cout << "Thread " << i 
                  << " duration: " << threadDurations[i] << " ms" 
                  << " processed: " << (chunkSize * NUM_ITERATIONS) / (1024.0 * 1024.0) << " MB" 
                  << std::endl;
    }

    // Calculate and display throughput
    double totalMB = totalBytesProcessed / (1024.0 * 1024.0);
    double throughput = totalMB / (totalDuration.count() / 1000.0);

    std::cout << "\nTotal Results:" << std::endl;
    std::cout << "Total time: " << totalDuration.count() << " ms" << std::endl;
    std::cout << "Total data processed: " << totalMB << " MB" << std::endl;
    std::cout << "Throughput: " << throughput << " MB/s" << std::endl;

    return (0);
}