#ifndef LOCKFREE_STACK_H
#define LOCKFREE_STACK_H

#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

template <typename T>
struct stack_node {
  T data;
  std::atomic<stack_node<T>*> next;

  stack_node() : data{}, next(nullptr) {}
  explicit stack_node(const T& value) : data(value), next(nullptr) {}
};

template <typename T>
class lockfree_stack {
 public:
  void push(const T& val);
  stack_node<T>* pop();
  explicit lockfree_stack(size_t threads_num);
  ~lockfree_stack();

 private:
  std::atomic<stack_node<T>*> top_;
  std::map<std::thread::id, stack_node<T>*> hazard_ptrs_;
  std::map<std::thread::id, std::vector<stack_node<T>*>> retired_;
  size_t threads_num_;
  void retire(stack_node<T>* retired_ptr);
  void scan();
  void init_hazard_ptrs();
  std::mutex hazard_init_mutex_;
};

template <typename T>
void lockfree_stack<T>::push(const T& val) {
  auto* new_node = new stack_node<T>(val);
  // std::ostringstream os;
  // os << std::this_thread::get_id() << " Push with addr " << new_node << "\n";
  // std::cout << os.str();
  stack_node<T>* top = top_.load(std::memory_order_relaxed);
  while (true) {
    new_node->next.store(top, std::memory_order_relaxed);
    if (top_.compare_exchange_weak(top, new_node, std::memory_order_release)) {
      return;
    }
    std::this_thread::yield();
  }
}

template <typename T>
stack_node<T>* lockfree_stack<T>::pop() {
  init_hazard_ptrs();
  const std::thread::id tid = std::this_thread::get_id();
  retire(hazard_ptrs_[tid]);
  // std::ostringstream os;
  while (true) {
    stack_node<T>* top = top_.load(std::memory_order_acquire);
    hazard_ptrs_[tid] = top;
    if (!top) {
      // os << tid << " Pop empty\n";
      // std::cout << os.str();
      return nullptr;
    }
    stack_node<T>* next = top->next.load(std::memory_order_relaxed);
    if (top_.compare_exchange_weak(top, next, std::memory_order_acquire,
                                   std::memory_order_relaxed)) {
      // os << tid << " Pop  with addr " << top << "\n";
      // std::cout << os.str();
      return top;
    }
    std::this_thread::yield();
  }
}

// 0: A -> C
// 1: A -> B -> C

template <typename T>
lockfree_stack<T>::lockfree_stack(size_t threads_num)
    : threads_num_(threads_num), top_(nullptr) {}

template <typename T>
lockfree_stack<T>::~lockfree_stack() {
  for (auto it = hazard_ptrs_.begin(); it != hazard_ptrs_.end(); ++it) {
    delete it->second;
  }
  for (auto it = retired_.begin(); it != retired_.end(); ++it) {
    for (auto& retired : it->second) {
      delete retired;
    }
  }
}

template <typename T>
void lockfree_stack<T>::init_hazard_ptrs() {
  std::thread::id tid = std::this_thread::get_id();
  std::lock_guard<std::mutex> guard(hazard_init_mutex_);
  if (hazard_ptrs_.find(tid) == hazard_ptrs_.end()) {
    hazard_ptrs_[tid] = nullptr;
    retired_[tid] = std::vector<stack_node<T>*>();
  }
}

template <typename T>
void lockfree_stack<T>::retire(stack_node<T>* retired_ptr) {
  if (!retired_ptr) {
    return;
  }
  std::thread::id tid = std::this_thread::get_id();
  retired_[tid].push_back(retired_ptr);
  if (retired_[tid].size() >= 2 * threads_num_) {
    scan();
  }
}

template <typename T>
void lockfree_stack<T>::scan() {
  std::vector<stack_node<T>*> all_hazard;
  std::vector<stack_node<T>*> updated_retired;
  std::thread::id tid = std::this_thread::get_id();
  for (auto it = hazard_ptrs_.begin(); it != hazard_ptrs_.end(); ++it) {
    all_hazard.push_back(it->second);
  }
  std::sort(all_hazard.begin(), all_hazard.end());
  auto& all_retired = retired_[tid];
  for (auto& retired : all_retired) {
    if (std::binary_search(all_hazard.begin(), all_hazard.end(), retired)) {
      updated_retired.push_back(retired);
    } else {
      // std::ostringstream os;
      // os << all_retired.size() << " " << tid << " Delete retired " << retired
      //    << "\n";
      // std::cout << os.str();
      delete retired;
    }
  }
  retired_[tid] = updated_retired;
}

#endif  // LOCKFREE_STACK_H
