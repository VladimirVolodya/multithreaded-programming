#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include "spin-lock.h"
#include "ticket-lock.h"

#if defined(SPIN_LOCK)
#define mutex_t spin_lock_TTAS
#elif defined(TICKET_LOCK)
#define mutex_t ticket_lock
#else
#define mutex_t std::mutex
#endif
#define SLEEP_FOR_MILLISECONDS 100

void job(mutex_t& mutex, uint64_t& elapsed) {
  auto start = std::chrono::steady_clock::now();
  mutex.lock();
  auto finish = std::chrono::steady_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start)
                .count();
  std::thread::id pid = std::this_thread::get_id();
  std::cout << "[" << pid << "] Going to sleep..." << std::endl;
  std::this_thread::sleep_for(
      std::chrono::milliseconds(SLEEP_FOR_MILLISECONDS));
  std::cout << "[" << pid << "] Woke up!" << std::endl;
  mutex.unlock();
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: ./" << argv[0] << " <threads_number>" << std::endl;
  }
  int threads_num = atoi(argv[1]);
  mutex_t mutex;
  std::vector<std::thread> threads;
  std::vector<uint64_t> elapsed_times(threads_num);
  threads.reserve(threads_num);
  for (size_t i = 0; i < threads_num; ++i) {
    threads.push_back(std::move(
        std::thread(job, std::ref(mutex), std::ref(elapsed_times[i]))));
  }
  for (auto& thread : threads) {
    thread.join();
  }
  std::sort(elapsed_times.begin(), elapsed_times.end());
  for (uint32_t i = 0; i < elapsed_times.size(); ++i) {
    elapsed_times[i] -= i * SLEEP_FOR_MILLISECONDS * 1000000;
  }
  uint64_t max = *max_element(elapsed_times.begin(), elapsed_times.end());
  uint64_t mean =
      std::accumulate(elapsed_times.begin(), elapsed_times.end(), 0) /
      elapsed_times.size();
  std::cout << "Max: " << max << std::endl;
  std::cout << "Mean: " << mean << std::endl;
  return 0;
}
