#include <thread>
#include <vector>
#include <chrono>
#include <iostream>

using namespace std::chrono;

const int NUM_THREADS = 22;
const size_t BUFFER_SIZE = 10000000000;
char* buffer = new char[BUFFER_SIZE];

void memory_worker(size_t start, size_t size, int thread_id) {
    auto start_time = steady_clock::now();
    size_t bytes_processed = 0;

    // Simulate continuous read/write
    for (size_t i = 0; i < size; i += 64) {
        volatile char temp = buffer[start + i];
        buffer[start + i + 32] = temp + 1;
        bytes_processed += 64;
    }

    auto end_time = steady_clock::now();
    double time_seconds = duration<double>(end_time - start_time).count();
    double throughput = (bytes_processed / time_seconds) / (1024 * 1024); // MB/s
    std::cout << "Thread " << thread_id << " Throughput: " << throughput << " MB/s\n";
}

int main() {
    std::vector<std::thread> threads;

    size_t chunk_size = BUFFER_SIZE / NUM_THREADS;
    for (int i = 0; i < NUM_THREADS; ++i) {
        size_t start = i * chunk_size;
        threads.emplace_back(memory_worker, start, chunk_size, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    delete[] buffer;
    return 0;
}