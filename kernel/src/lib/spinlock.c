#include <lib/spinlock.h>

void acquire(spinlock* spin) {
  while (atomic_flag_test_and_set_explicit(spin, memory_order_acquire));
}

void release(spinlock* spin) {
  atomic_flag_clear_explicit(spin, memory_order_release);
}