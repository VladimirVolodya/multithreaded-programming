#ifndef MY_TICKETLOCK
#define MY_TICKETLOCK

#include <atomic>

class ticket_lock {
  std::atomic_size_t now_serving = {0};
  std::atomic_size_t next_ticket = {0};

 public:
  void lock();
  void unlock();
};

#endif  // MY_TICKETLOCK
