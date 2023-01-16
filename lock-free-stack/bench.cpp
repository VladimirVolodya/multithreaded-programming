#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <thread>

#include "lock_free_stack.h"
#include "not_lockfree_stack.h"

#ifdef LOCK_FREE
#define stack_t lockfree_stack
#else
#define stack_t not_lockfree_stack
#endif

void job(stack_t<int>& stack, int64_t n_ops, int64_t& max, int64_t& mean) {
  std::thread::id tid = std::this_thread::get_id();
  int64_t sum = 0;
  for (int i = 0; i < n_ops; ++i) {
    if (rand() % 2) {
      auto start = std::chrono::steady_clock::now();
      stack.push(0);
      auto finish = std::chrono::steady_clock::now();
      int64_t elapsed =
          std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start)
              .count();
      sum += elapsed;
      if (elapsed > max) {
        max = elapsed;
      }
    } else {
      auto start = std::chrono::steady_clock::now();
      stack.pop();
      auto finish = std::chrono::steady_clock::now();
      int64_t elapsed =
          std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start)
              .count();
      sum += elapsed;
      if (elapsed > max) {
        max = elapsed;
      }
    }
  }
  mean = sum / n_ops;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: ./%s <num_of_threads> <num_of_operations>\n",
            argv[0]);
    return 1;
  }
  std::vector<std::thread> threads;
  int n_threads = atoi(argv[1]);
  int n_ops = atoi(argv[2]);
  std::vector<int64_t> maxes(n_threads);
  std::vector<int64_t> means(n_threads);
  stack_t<int> stack(n_threads);
  for (int i = 0; i < n_threads; ++i) {
    threads.emplace_back(std::thread(job, std::ref(stack), n_ops,
                                     std::ref(maxes[i]), std::ref(means[i])));
  }
  for (auto& t : threads) {
    t.join();
  }
  std::cout << *max_element(maxes.cbegin(), maxes.cend()) << ' '
            << std::accumulate(means.cbegin(), means.cend(), 0) / n_threads
            << '\n';
  return 0;
}
