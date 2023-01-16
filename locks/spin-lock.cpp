#include "spin-lock.h"

#include <thread>

spin_lock_TTAS::spin_lock_TTAS() : m_spin(0) {}

spin_lock_TTAS::~spin_lock_TTAS() {
  assert(m_spin.load(std::memory_order_relaxed) == 0);
}

void spin_lock_TTAS::lock() {
  unsigned int expected;
  do {
    while (m_spin.load(std::memory_order_relaxed)) {
      std::this_thread::yield();
    }
    expected = 0;
  } while (
      !m_spin.compare_exchange_weak(expected, 1, std::memory_order_acquire));
}

void spin_lock_TTAS::unlock() { m_spin.store(0, std::memory_order_release); }
