#ifndef TIMING_CORE_H
#define TIMING_CORE_H

#include <stdint.h>

class TimingCore {
 public:
  enum State { READY, RUNNING, FINISHED, TIMED_OUT };
  // Gercek en kisa turdan kucuk olmali. Birimi mikrosaniye.
  static constexpr uint32_t MIN_GAP_US = 500000UL;
  // micros() tasmasindan once olcumu sonlandir (60 dakika).
  static constexpr uint32_t MAX_RUN_US = 3600000000UL;

  void reset(uint32_t now) {
    state_ = READY;
    transition_ = now;
    start_ = now;
  }

  bool canArm(uint32_t now) const {
    return (state_ == READY || state_ == RUNNING) &&
           uint32_t(now - transition_) >= MIN_GAP_US;
  }

  void crossing(uint32_t now) {
    if (!canArm(now)) return;
    if (state_ == READY) {
      start_ = now;
      transition_ = now;
      state_ = RUNNING;
      return;
    }
    const uint32_t duration = now - start_;
    if (duration >= MAX_RUN_US) {
      state_ = TIMED_OUT;
      return;
    }
    last_ = duration;
    if (!hasLast_ || duration < best_) best_ = duration;
    hasLast_ = true;
    state_ = FINISHED;
    transition_ = now;
  }

  void tick(uint32_t now) {
    if (state_ == RUNNING && uint32_t(now - start_) >= MAX_RUN_US) {
      state_ = TIMED_OUT;
    }
  }

  State state() const { return state_; }
  bool hasLast() const { return hasLast_; }
  uint32_t last() const { return last_; }
  uint32_t best() const { return best_; }
  uint32_t elapsed(uint32_t now) const {
    if (state_ == RUNNING) return uint32_t(now - start_);
    if (state_ == FINISHED) return last_;
    if (state_ == TIMED_OUT) return MAX_RUN_US;
    return 0;
  }

 private:
  State state_ = READY;
  uint32_t start_ = 0;
  uint32_t transition_ = 0;
  uint32_t last_ = 0;
  uint32_t best_ = 0;
  bool hasLast_ = false;
};

#endif
