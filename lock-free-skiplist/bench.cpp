#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <thread>

#include "lock_free_skiplist.h"

#define GLIBCXX_USE_CXX11_ABI 0

#ifdef LOCKABLE
#define skiplist_t LockFreeSkiplist<int>
#else
#define skiplist_t LockFreeSkiplist<int>
#endif

#define ITER 10000
#define MAX_NUM 1000

void job(skiplist_t& skiplist, std::string name, int64_t& mean, int64_t& max) {
  int64_t sum = 0, tmp_max = 0;
  for (int i = 0; i < ITER; ++i) {
    std::ostringstream os1;
    std::ostringstream os2;
    int val = rand() % MAX_NUM;
    // skiplist.print_nexts();
    // std::this_thread::sleep_for(std::chrono::nanoseconds(10));
    auto start = std::chrono::steady_clock::now();
    switch (rand() % 3) {
      case 0:
        os1 << name << " adding " << val << std::endl;
        std::cout << os1.str();
        os2 << name << " added " << val << ' ' << skiplist.add(val)
            << std::endl;
        // skiplist.add(val);
        break;
      case 1:
        os1 << name << " checking " << val << std::endl;
        std::cout << os1.str();
        os2 << name << " checked if contains " << val << ' '
            << skiplist.contains(val) << std::endl;
        // skiplist.contains(val);
        break;
      case 2:
        os1 << name << " removing " << val << std::endl;
        std::cout << os1.str();
        os2 << name << " removed " << val << ' ' << skiplist.remove(val)
            << std::endl;
        // skiplist.remove(val);
        break;
    }
    std::cout << os2.str();
    auto finish = std::chrono::steady_clock::now();
    int64_t duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start)
            .count();
    sum += duration;
    if (duration > tmp_max) {
      tmp_max = duration;
    }
    // skiplist.print_nexts();
    // std::cout << '\n';
  }
  mean = sum / ITER;
  max = tmp_max;
}

int main(int argc, char* argv[]) {
  LockFreeSkiplist<int> vals(64);
  if (argc != 2) {
    std::cerr << "Usage: ./" << argv[0] << " <num_of_threads>" << std::endl;
    return 1;
  }
  size_t threads_num = strtoull(argv[1], nullptr, 10);
  std::vector<std::thread> threads;
  std::vector<int64_t> means(threads_num, 0);
  std::vector<int64_t> maxes(threads_num, 0);
  threads.reserve(threads_num);
  for (size_t i = 0; i < threads_num; ++i) {
    threads.emplace_back(job, std::ref(vals),
                         std::string("t") + std::to_string(i),
                         std::ref(means[i]), std::ref(maxes[i]));
  }
  for (auto& t : threads) {
    t.join();
    // std::cout << "joined\n";
  }
  std::cout << "\nResults:\n"
            << std::accumulate(means.begin(), means.end(), 0) / threads_num
            << ' ' << *max_element(maxes.begin(), maxes.end()) << std::endl;
  return 0;
}
