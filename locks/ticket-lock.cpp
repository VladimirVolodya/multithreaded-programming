#include "ticket-lock.h"

#include <thread>

void ticket_lock::lock() {
  const auto ticket = next_ticket.fetch_add(1, std::memory_order_relaxed);
  while (now_serving.load(std::memory_order_acquire) != ticket) {
    std::this_thread::yield();
  }
}

void ticket_lock::unlock() {
  const auto successor = now_serving.load(std::memory_order_relaxed) + 1;
  now_serving.store(successor, std::memory_order_release);
}
