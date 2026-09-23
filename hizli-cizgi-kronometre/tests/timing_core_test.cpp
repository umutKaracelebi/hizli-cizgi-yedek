#include <cassert>
#include <cstdint>
#include <iostream>
#include "../robot_kronometre/TimingCore.h"

int main() {
  TimingCore timer;
  timer.reset(0);
  timer.crossing(499999);
  assert(timer.state() == TimingCore::READY);
  timer.crossing(500000);
  assert(timer.state() == TimingCore::RUNNING);
  timer.crossing(999999);
  assert(timer.state() == TimingCore::RUNNING);
  timer.crossing(1750123);
  assert(timer.state() == TimingCore::FINISHED);
  assert(timer.last() == 1250123);
  assert(timer.best() == 1250123);
  timer.crossing(2500000);
  assert(timer.last() == 1250123); // Bitince tekrar baslamaz.

  timer.reset(3000000);
  assert(timer.hasLast()); // Buton kayitlari silmez.
  assert(timer.elapsed(3000000) == 0);
  timer.crossing(3500000);
  timer.crossing(4500000);
  assert(timer.last() == 1000000);
  assert(timer.best() == 1000000);
  timer.reset(5000000);
  timer.crossing(5500000);
  timer.crossing(7500000);
  assert(timer.last() == 2000000);
  assert(timer.best() == 1000000);

  // micros() sayacinin bir tur tasmasi dogru ele alinmali.
  TimingCore wrap;
  wrap.reset(UINT32_MAX - 2000000UL);
  const uint32_t start = UINT32_MAX - 1000000UL;
  wrap.crossing(start);
  wrap.crossing(uint32_t(start + 1500000UL));
  assert(wrap.last() == 1500000UL);

  TimingCore timeout;
  timeout.reset(0);
  timeout.crossing(500000);
  timeout.tick(uint32_t(500000UL + TimingCore::MAX_RUN_US));
  assert(timeout.state() == TimingCore::TIMED_OUT);
  assert(!timeout.hasLast());
  assert(!timeout.canArm(4000000000UL));
  timeout.reset(4000000000UL);
  assert(timeout.state() == TimingCore::READY);

  TimingCore lateEdge;
  lateEdge.reset(0);
  lateEdge.crossing(500000);
  lateEdge.crossing(uint32_t(500000UL + TimingCore::MAX_RUN_US));
  assert(lateEdge.state() == TimingCore::TIMED_OUT);
  assert(!lateEdge.hasLast());

  timer.reset(8000000);
  timer.crossing(8500000);
  timer.reset(9000000); // Calisan olcumu iptal et.
  assert(timer.state() == TimingCore::READY);
  assert(timer.last() == 2000000);
  std::cout << "TimingCore tests passed\n";
}
