#ifndef NOT_LOCKFREE_STACK_H
#define NOT_LOCKFREE_STACK_H

#include <mutex>

template <typename T>
struct nstack_node {
  T data;
  nstack_node<T>* next;

  nstack_node() : data{}, next(nullptr) {}
  explicit nstack_node(const T& value) : data(value), next(nullptr) {}
};

template <typename T>
class not_lockfree_stack {
 public:
  not_lockfree_stack(int) : top_(nullptr) {}
  void push(const T& val);
  T pop();

 private:
  std::mutex lock_;
  nstack_node<T>* top_;
};

template <typename T>
void not_lockfree_stack<T>::push(const T& val) {
  std::lock_guard<std::mutex> guard(lock_);
  nstack_node<T>* new_top = new nstack_node<T>(val);
  new_top->next = top_;
  top_ = new_top;
}

template <typename T>
T not_lockfree_stack<T>::pop() {
  std::lock_guard<std::mutex> guard(lock_);
  if (!top_) {
    return 0;
  }
  T val = top_->data;
  nstack_node<T>* prev_top = top_;
  top_ = top_->next;
  delete prev_top;
  return val;
}

#endif  // NOT_LOCKFREE_STACK_H
