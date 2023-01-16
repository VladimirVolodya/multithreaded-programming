#ifndef MY_SPINLOCK
#define MY_SPINLOCK

#include <atomic>
#include <cassert>

class spin_lock_TTAS {
  std::atomic<unsigned int> m_spin;

 public:
  spin_lock_TTAS();
  ~spin_lock_TTAS();
  void lock();
  void unlock();
};

#endif  // MY_SPINLOCK
