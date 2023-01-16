#ifndef MY_LOCKFREE_SKIPLIST
#define MY_LOCKFREE_SKIPLIST

#include <atomic>
#include <cstddef>
#include <iostream>
#include <limits>
#include <mutex>
#include <random>
#include <stack>
#include <thread>

template <typename T>
class LockFreeSkiplist {
 private:
  template <typename U>
  class Node;
  template <typename U>
  class MemoryManager;
  template <size_t init_time, size_t multiplier>
  class ExpBackoff;

 public:
  LockFreeSkiplist(ssize_t max_level)
      : max_level(max_level),
        head(memory_manager.alloc(std::numeric_limits<T>::min(), max_level,
                                  max_level)),
        tail(memory_manager.alloc(std::numeric_limits<T>::max(), max_level,
                                  max_level)) {
    for (ssize_t i = 0; i <= max_level; ++i) {
      head->next[i].store(MarkablePointer<Node<T>>(tail));
    }
  }
  ~LockFreeSkiplist() {
    Node<T>* cur = head;
    while (cur) {
      Node<T>* next = cur->next[0].load().getPtr();
      delete cur;
      cur = next;
    }
  }

  void print_nexts() {
    Node<T>* curr = head;
    while (curr) {
      std::cout << curr << ' ' << curr->val << ' ';
      for (ssize_t i = 0; i <= max_level; ++i) {
        std::cout << curr->next[i].load().getPtr() << ' ';
      }
      std::cout << '\n';
      curr = curr->next[0].load().getPtr();
    }
  }

  bool add(const T& val) {
    ssize_t top_level = random_level();
    ssize_t bottom_level = 0;
    Node<T>** preds = new Node<T>*[max_level + 1];
    Node<T>** succs = new Node<T>*[max_level + 1];
    ExpBackoff<1, 2> backoff;
    while (true) {
      if (find(val, preds, succs)) {
        delete[] preds;
        delete[] succs;
        return false;
      } else {
        // std::cout << '\n';
        // print_nexts();
        // std::cout << '\n';
        // Node<T>* new_node = memory_manager.alloc(max_level);
        // new_node->setVal(val);
        // new_node->setHeight(top_level);
        Node<T>* new_node = new Node<T>(val, top_level, max_level);
        // std::cout << '\n';
        // print_nexts();
        // std::cout << '\n';
        // if (val == 3) {
        //   return true;
        // }
        for (ssize_t level = bottom_level; level <= top_level; ++level) {
          Node<T>* succ = succs[level];
          new_node->next[level].store(MarkablePointer<Node<T>>(succ));
          // std::cout << new_node->next[level].load().getPtr() << ' ';
        }
        Node<T>* pred = preds[bottom_level];
        Node<T>* succ = succs[bottom_level];
        new_node->next[bottom_level].store(MarkablePointer<Node<T>>(succ));
        MarkablePointer<Node<T>> markable_succ(succ);
        if (!pred->next[bottom_level].compare_exchange_strong(
                markable_succ, MarkablePointer<Node<T>>(new_node))) {
          backoff();  // ok
          continue;
        }
        for (ssize_t level = bottom_level + 1; level <= top_level; ++level) {
          while (true) {
            pred = preds[level];
            succ = succs[level];
            MarkablePointer<Node<T>> markable_succ(succ);
            if (pred->next[level].compare_exchange_strong(
                    markable_succ, MarkablePointer<Node<T>>(new_node))) {
              break;
            }
            find(val, preds, succs);
          }
        }
        delete[] preds;
        delete[] succs;
        return true;
      }
    }
  }

  bool remove(const T& val) {
    ssize_t bottom_level = 0;
    Node<T>** preds = new Node<T>*[max_level + 1];
    Node<T>** succs = new Node<T>*[max_level + 1];
    MarkablePointer<Node<T>> succ;
    ExpBackoff<1, 2> backoff;
    while (true) {
      if (!find(val, preds, succs)) {
        delete[] preds;
        delete[] succs;
        return false;
      } else {
        Node<T>* node_to_remove = succs[bottom_level];
        for (ssize_t level = node_to_remove->top_level;
             level >= bottom_level + 1; --level) {
          succ = node_to_remove->next[level].load();
          bool flag = false;
          while (!succ.getMark()) {
            // if (flag) {
            //   backoff();
            // }
            node_to_remove->next[level].compare_exchange_strong(
                succ, MarkablePointer<Node<T>>(succ.getPtr(), true));
            flag = true;
          }
        }
        succ = node_to_remove->next[bottom_level].load();
        while (true) {
          MarkablePointer<Node<T>> succ_buf = succ.getPtr();
          bool i_marked_it =
              node_to_remove->next[bottom_level].compare_exchange_strong(
                  succ_buf, MarkablePointer<Node<T>>(succ.getPtr(), true));
          succ = succs[bottom_level]->next[bottom_level].load();
          if (i_marked_it) {
            find(val, preds, succs);
            delete[] preds;
            delete[] succs;
            memory_manager.retire(node_to_remove);
            return true;
          } else if (succ.getMark()) {
            delete[] preds;
            delete[] succs;
            return false;
          }
        }
      }
    }
  }

  bool contains(const T& val) {
    ssize_t bottom_level = 0;
    Node<T>* pred = head;
    Node<T>* curr;
    MarkablePointer<Node<T>> succ;
    ExpBackoff<1, 2> backoff;
    for (ssize_t level = max_level; level >= bottom_level; --level) {
      curr = pred->next[level].load().getPtr();
      while (true) {
        succ = curr->next[level].load();
        while (succ.getMark()) {
          // backoff();
          curr = pred->next[level].load().getPtr();
          succ = curr->next[level].load();
        }
        if (curr->val < val) {
          pred = curr;
          curr = succ.getPtr();
        } else {
          break;
        }
      }
    }
    return (curr->val == val);
  }

 private:
  MemoryManager<Node<T>> memory_manager;
  ssize_t max_level;
  Node<T>* const head;
  Node<T>* const tail;
  // MarkablePointer class
  template <typename U>
  class MarkablePointer {
   public:
    MarkablePointer(U* ptr = NULL, bool mark = false) {
      val = ((uintptr_t)ptr & ~mask) | (mark ? 1 : 0);
    }
    U* getPtr() { return (U*)(val & ~mask); }
    bool getMark() { return val & mask; }

   private:
    uintptr_t val;
    static const uintptr_t mask = 1;
  };
  template <typename U>
  using AtomicMarkablePointer = std::atomic<MarkablePointer<U>>;
  // end

  // Node class
  template <typename U>
  class Node {
   public:
    AtomicMarkablePointer<Node<U>>* next;
    U val;
    ssize_t top_level;
    Node(ssize_t max_level)
        : next(new AtomicMarkablePointer<Node<U>>[max_level + 1]) {
      for (ssize_t i = 0; i < max_level; ++i) {
        next[i].store(MarkablePointer<Node<U>>());
      }
    }
    Node(const U& val, ssize_t height, ssize_t max_level) : Node(max_level) {
      this->val = val;
      this->top_level = height;
    }
    void setVal(const U& val) { this->val = val; }
    void setHeight(const ssize_t& height) { this->top_level = height; }
    ~Node() { delete[] next; }
  };
  // end

  // MemoryManager class
  template <typename U>
  class MemoryManager {
   public:
    template <typename... Args>
    U* alloc(Args&&... args) {
      std::lock_guard<std::mutex> guard(lock);
      if (buf.empty()) {
        buf.push(new U(std::forward<Args>(args)...));
      } else {
        *buf.top() = U(std::forward<Args>(args)...);
      }
      U* top = buf.top();
      buf.pop();
      return top;
    }
    void retire(U* p) {
      std::lock_guard<std::mutex> guard(lock);
      buf.push(p);
    }
    ~MemoryManager() {
      while (!buf.empty()) {
        delete buf.top();
        buf.pop();
      }
    }

   private:
    std::stack<U*> buf;
    std::mutex lock;
  };
  // end

  // ExpBackoff class
  template <size_t init_time, size_t multiplier>
  class ExpBackoff {
   public:
    ExpBackoff() : cur_sleep_for(init_time) {}
    void operator()() {
      std::this_thread::sleep_for(std::chrono::nanoseconds(cur_sleep_for));
      cur_sleep_for *= multiplier;
    }

   private:
    size_t cur_sleep_for;
  };
  // end

  ssize_t random_level() {
    ssize_t level = 0;
    while (level < max_level) {
      if (rand() % 2) {
        break;
      }
      ++level;
    }
    return level;
  }

  bool find(const T& val, Node<T>** preds, Node<T>** succs) {
    ssize_t bottom_level = 0;
    Node<T>* pred;
    Node<T>* curr;
    MarkablePointer<Node<T>> succ;
    ExpBackoff<1, 2> backoff;
  retry:
    while (true) {
      pred = head;
      for (ssize_t level = max_level; level >= bottom_level; --level) {
        curr = pred->next[level].load().getPtr();
        while (true) {
          succ = curr->next[level].load();
          while (succ.getMark()) {
            MarkablePointer<Node<T>> markable_curr(curr);
            if (!pred->next[level].compare_exchange_strong(
                    markable_curr, MarkablePointer<Node<T>>(succ.getPtr()))) {
              // std::cout << "goto\n";
              backoff();  // ok
              goto retry;
            }
            curr = pred->next[level].load().getPtr();
            succ = curr->next[level].load();
          }
          if (curr->val < val) {
            pred = curr;
            curr = succ.getPtr();
          } else {
            break;
          }
        }
        preds[level] = pred;
        succs[level] = curr;
      }
      return curr->val == val;
    }
  }
};

#endif  // MY_LOCKFREE_SKIPLIST