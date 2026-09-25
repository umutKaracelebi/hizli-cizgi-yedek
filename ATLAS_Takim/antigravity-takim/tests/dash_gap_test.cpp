/*
  dash_gap_test.cpp - antigravity-takim "kesikli cizgi" (dash) davranis testi

  Amac
  ----
  Gercek firmware sekmelerini (antigravity-takim.ino, Motors.ino,
  RunControl.ino, Sensors.ino, UI.ino) oldugu gibi dahil eder; Arduino/AVR
  katmanini tests/stubs/Arduino.h ile taklit eder. Boylece karta yukleme
  yapmadan kontrol dongusu (updatePeriod) ve kopru/arama durum makinesi
  adim adim kosturulur ve su sorular kanitla yanitlanir:

   1) Duz kesikli cizgide beyaz bosluk gecilirken robot DONUYOR MU?
      (Beklenen: hayir - kopru boyunca duz devam.)
   2) Keskin donus/kose kaybinda arama manevrasi hala calisiyor mu?
      (Beklenen: evet - kisa kopru + yumusak pivot.)
   3) Kesik sirasinda barin ucundan gorunen yabanci cizgi kabul ediliyor mu?
      (Beklenen: hayir - merkez bandi kapisi.)
   4) Cizgi tamamen biterse robot kacip yabanci cizgiye kilitleniyor mu?
      (Beklenen: hayir - iki yonlu yumusak arama + failsafe freni.)

  Sinirlar
  --------
  Model kabadir: PWM -> hiz iliskisi dogrusal; enkoder/patinaj/vakum etkisi,
  motor zaman sabiti ve gercek gecikmeler yoktur. Sonuc "gercek pistte
  dogrulandi" anlamina gelmez; kontrol mantiginin regresyon testidir.

  Derleme ve kosum (proje kokunde; g++ veya clang++ gerekir)
  ---------------------------------------------------------
    g++ -std=c++11 -Wall -Wextra -pedantic -o atlas_dash_test.exe ^
        ATLAS_Takim/antigravity-takim/tests/dash_gap_test.cpp
*/

#include "stubs/Arduino.h"

#include "atlas_prototypes.h"

// Arduino derleyicisiyle ayni sira: ana sekme, sonra diger sekmeler (alfabetik).
#include "../antigravity-takim.ino"
#include "../Motors.ino"
#include "../RunControl.ino"
#include "../Sensors.ino"
#include "../UI.ino"

//  =====================================================================
//   S I M U L A S Y O N   M O D E L I
//  =====================================================================

namespace sim {

// --- Kaba fizik parametreleri (varsayim; olcumle guncellenebilir) ---
const double PWM_TO_MM_PER_MS = 0.01;   // PWM 100 -> 1.0 mm/ms (yaklasik 1 m/s)
const double WHEEL_BASE_MM = 100.0;     // tekerlek acikligi (robot 170x245 mm)
const double SENSOR_PITCH_MM = 8.0;     // 16 sensor x 8 mm = 120 mm bar (varsayim)
const double LINE_HALF_WIDTH_MM = 10.0; // pist cizgisi ~20 mm (pist1.pdf)

// --- Pist modeli ---
double dashLenMm = 100.0;   // siyah parca uzunlugu
double gapLenMm = 55.0;     // beyaz bosluk (pist1 olcumu ~55.7 mm)
double lineLatOffset = 0.0; // pist cizgisinin yanal konumu (yabanci cizgi testi icin)
bool trackEnabled = true;   // false: cizgi yok
bool lineEndsAtY = false;   // true: lineEndY'den sonra cizgi tamamen biter
double lineEndY = 0.0;
bool foreignLine = false;   // true: "pist disi" ikinci (yabanci) cizgi
double foreignLineLat = 0.0;

// --- Robot durumu (x: yanal, y: ilerleme, theta: yon hatasi) ---
double x = 0.0;
double y = 0.0;
double theta = 0.0;

double lastPwmL = 0.0;
double lastPwmR = 0.0;

bool hasBlackAtLat(double lat, double lon) {
  if (trackEnabled && !(lineEndsAtY && lon > lineEndY) &&
      fabs(lat - lineLatOffset) <= LINE_HALF_WIDTH_MM) {
    if (lon < 0.0) return false;
    const double period = dashLenMm + gapLenMm;
    const double withinDash = fmod(lon, period);
    return withinDash < dashLenMm;
  }
  if (foreignLine && fabs(lat - foreignLineLat) <= LINE_HALF_WIDTH_MM) return true;
  return false;
}

bool sensorSeesBlack(int i) {
  const double offset = ((double)i - 7.5) * SENSOR_PITCH_MM;
  const double lat = x + offset * cos(theta);
  const double lon = y - offset * sin(theta);
  return hasBlackAtLat(lat, lon);
}

void reset(double startX, double startY, double startTheta = 0.0) {
  x = startX;
  y = startY;
  theta = startTheta;
  lastPwmL = 0.0;
  lastPwmR = 0.0;
  trackEnabled = true;
  lineEndsAtY = false;
  foreignLine = false;
  lineEndY = 0.0;
  foreignLineLat = 0.0;
  dashLenMm = 100.0;
  gapLenMm = 55.0;
  lineLatOffset = 0.0;
}

// --- Kayit: her kontrol dongusu icin teshis izi ---
// Masaustu kossunda tam boyut kullanilir. AVR capraz derleyicisiyle yalnizca
// sozdizimi kontrolu yapilirsa kucuk bir deger verilir (or. -DSIM_MAX_RECORDS=80),
// cunku ATmega328P'nin 2 KB SRAM'ine 4000 kayit sigmaz.
#ifndef SIM_MAX_RECORDS
#define SIM_MAX_RECORDS 4000
#endif
const int MAX_RECORDS = SIM_MAX_RECORDS;
struct Rec {
  unsigned long t;
  bool onLine;
  bool episode;  // offlineEpisodeActive
  bool isDash;   // offlineIsDash
  int pwmL;
  int pwmR;
  int x10;       // 0.1 mm biriminde yanal konum
  int yawDeg;    // derece
};
Rec records[MAX_RECORDS];
int recordCount = 0;

void capture(unsigned long nowMs) {
  if (recordCount >= MAX_RECORDS) return;
  Rec &r = records[recordCount++];
  r.t = nowMs;
  r.onLine = isOnLine;
  r.episode = offlineEpisodeActive;
  r.isDash = offlineIsDash;
  r.pwmL = outputPWML;
  r.pwmR = outputPWMR;
  r.x10 = (int)(x * 10.0);
  r.yawDeg = (int)(theta * 180.0 / 3.14159265);
}

// Tek kontrol dongusu + kaba fizik adimi (dt = 1 ms).
void step(unsigned long nowMs, int dtMs = 1) {
  for (int i = 0; i < TOTAL_SENSORS; ++i) {
    TestBoard::adcRaw[i] = sensorSeesBlack(i) ? 0 : 255;  // siyah = 0 (ters mod)
  }
  TestBoard::nowUs = (uint64_t)nowMs * 1000ULL;

  currentTime = nowMs;
  elapsedTime = currentTime - startTime;

  updatePeriod();

  capture(nowMs);

  lastPwmL = outputPWML;
  lastPwmR = outputPWMR;

  const double vL = lastPwmL * PWM_TO_MM_PER_MS;
  const double vR = lastPwmR * PWM_TO_MM_PER_MS;
  const double v = 0.5 * (vL + vR);
  const double omega = (vL - vR) / WHEEL_BASE_MM;  // +x yonune donus

  theta += omega * (double)dtMs;
  x += v * sin(theta) * (double)dtMs;
  y += v * cos(theta) * (double)dtMs;
}

const Rec *at(unsigned long t) {
  for (int i = 0; i < recordCount; ++i) {
    if (records[i].t == t) return &records[i];
  }
  return 0;
}

int maxAbsX10(int fromMs, int toMs) {
  int worst = 0;
  for (int i = 0; i < recordCount; ++i) {
    if ((int)records[i].t < fromMs || (int)records[i].t > toMs) continue;
    const int value = records[i].x10 < 0 ? -records[i].x10 : records[i].x10;
    if (value > worst) worst = value;
  }
  return worst;
}

int maxAbsYawDeg(int fromMs, int toMs) {
  int worst = 0;
  for (int i = 0; i < recordCount; ++i) {
    if ((int)records[i].t < fromMs || (int)records[i].t > toMs) continue;
    const int value = records[i].yawDeg < 0 ? -records[i].yawDeg : records[i].yawDeg;
    if (value > worst) worst = value;
  }
  return worst;
}

// Kayip olaylarinin sayisi (false -> true gecisleri)
int episodeCount(int fromMs, int toMs) {
  int count = 0;
  bool previous = false;
  for (int i = 0; i < recordCount; ++i) {
    if ((int)records[i].t < fromMs || (int)records[i].t > toMs) continue;
    if (records[i].episode && !previous) count++;
    previous = records[i].episode;
  }
  return count;
}

}  // namespace sim

// Testlerde kisa ad kullanimi (sim::Rec -> Rec)
using sim::Rec;

//  =====================================================================
//   O R T A K   H A Z I R L I K
//  =====================================================================

// Kalibrasyonu dogrudan kurar (calibrateSensors() buton bekledigi icin):
// maxSensorValues = 255, minSensorValues = 0 -> esik 127, ters (siyah) mod.
void setupCalibration() {
  invertSensorReads = true;  // SW2 = siyah cizgi modu (bizim pist)
  for (int i = 0; i < TOTAL_SENSORS; ++i) {
    maxSensorValues[i] = 255;
    minSensorValues[i] = 0;
    sensorThreshold[i] = 127;
  }
}

// Kossu baslatir: zaman tabanini sifirlar ve runInit() cagirir.
void beginRun() {
  TestBoard::nowUs = 0;
  sim::recordCount = 0;
  setupCalibration();
  runInit();          // startTime = 0, velocityPWM korunur
  velocityPWM = BASE_VELOCITY_PWM;
  sim::step(0);       // ilk kontrol dongusu
}

//  =====================================================================
//   T1 - Duz kesikli cizgi gecisi (kullanicinin bildirdigi senaryo)
//  =====================================================================
void testStraightDashCrossing() {
  sim::reset(0.0, 10.0);
  sim::dashLenMm = 200.0;    // ilk beyaz bosluk hiz rampasindan sonra gelsin
  sim::gapLenMm = 55.0;      // pist1 olcumu (~55.7 mm)
  sim::lineLatOffset = 5.0;  // hafif kacik baslangic (gercekci)
  beginRun();

  int t = 0;
  bool settled = false;
  while (t < 3000) {
    ++t;
    sim::step((unsigned long)t);
    if (sim::episodeCount(0, t) >= 3 && isOnLine && !offlineEpisodeActive) {
      settled = true;
      break;
    }
  }

  int episodeRecords = 0, dashRecords = 0, negativePwmRecords = 0, maxSteer = 0;
  for (int i = 0; i < sim::recordCount; ++i) {
    const Rec &r = sim::records[i];
    if (!r.episode) continue;
    episodeRecords++;
    if (r.isDash) dashRecords++;
    if (r.pwmL < 0 || r.pwmR < 0) negativePwmRecords++;
    const int steer = r.pwmL - r.pwmR;
    if (steer > maxSteer) maxSteer = steer;
    if (-steer > maxSteer) maxSteer = -steer;
  }

  const int gaps = sim::episodeCount(0, t);
  const int worstX = sim::maxAbsX10(0, t);
  const int worstYaw = sim::maxAbsYawDeg(0, t);

  printf("[T1] t=%d ms, beyaz bosluk=%d, kayip dongusu=%d (dash=%d), negatif PWM dongusu=%d, max direksiyon=%d\n",
         t, gaps, episodeRecords, dashRecords, negativePwmRecords, maxSteer);
  printf("[T1] max |x|=%d.%d mm, max |yaw|=%d derece\n", worstX / 10, worstX % 10, worstYaw);

  assert(settled);                        // bosluklar gecildi, cizgi yeniden bulundu
  assert(gaps >= 3);                      // kesikler gercekten deneyimlendi
  assert(dashRecords == episodeRecords);  // hepsi "duz kesik" olarak siniflandi
  assert(negativePwmRecords == 0);        // boslukta pivot/donus YOK (duz devam)
  assert(maxSteer <= 2 * OFFLINE_DASH_HOLD_STEER_MAX);
  assert(worstX <= 400);                  // 40 mm: seritten cikilmadi
  assert(worstYaw <= 15);                 // beyaz boslukta belirgin donus yok
}

//  =====================================================================
//   T2 - Kopru penceresi: 15 ms'de pivot yok, 120 ms'de arama baslar
//  =====================================================================
void testDashBridgeWindow() {
  sim::reset(0.0, 10.0);
  sim::dashLenMm = 100000.0;  // baslangicta kesik yok (tek parca cizgi)
  sim::gapLenMm = 100000.0;
  beginRun();

  int t = 0;
  while (t < 1000 && !offlineEpisodeActive) {
    ++t;
    if (sim::y > 200.0) {  // hiz rampasi (150 ms) bitsin
      sim::lineEndsAtY = true;
      sim::lineEndY = sim::y - 1.0;
    }
    sim::step((unsigned long)t);
  }
  assert(offlineEpisodeActive);
  assert(offlineIsDash);  // merkezde ve duz gidiyorduk -> kesik varsayimi
  const int tLoss = t;

  for (int i = 1; i <= 140; ++i) sim::step((unsigned long)(tLoss + i));

  const Rec *early = sim::at((unsigned long)(tLoss + OFFLINE_GAP_BRIDGE_MS));
  const Rec *mid = sim::at((unsigned long)(tLoss + OFFLINE_DASH_BRIDGE_MS / 2));
  const Rec *late = sim::at((unsigned long)(tLoss + OFFLINE_DASH_BRIDGE_MS + 5));
  assert(early && mid && late);

  printf("[T2] kopru: tLoss=%d ms | +%d ms pwm=(%d,%d) | +%d ms pwm=(%d,%d) | +%d ms pwm=(%d,%d)\n",
         tLoss, OFFLINE_GAP_BRIDGE_MS, early->pwmL, early->pwmR,
         OFFLINE_DASH_BRIDGE_MS / 2, mid->pwmL, mid->pwmR,
         OFFLINE_DASH_BRIDGE_MS + 5, late->pwmL, late->pwmR);

  // Eski surumde 15 ms'de baslayan sert pivot artik YOK: iki teker de ileri.
  assert(early->pwmL > 0 && early->pwmR > 0);
  assert(mid->pwmL > 0 && mid->pwmR > 0);
  assert(early->yawDeg <= 3 && early->yawDeg >= -3);  // donus baslamadi
  assert(early->episode && mid->episode && late->episode);

  // Kopru suresi dolunca yerinde donusle arama baslar (simetrik, +-120).
  assert(late->pwmL == OFFLINE_SEARCH_PWM || late->pwmR == OFFLINE_SEARCH_PWM);
  assert(late->pwmL == -late->pwmR);
}

//  =====================================================================
//   T3 - Keskin donus/kose kaybi: arama manevrasi hala calisiyor mu?
//  =====================================================================
void testCornerLossStillPivots() {
  sim::reset(0.0, 10.0);
  sim::dashLenMm = 100000.0;
  sim::gapLenMm = 100000.0;
  beginRun();

  int t = 0;
  while (t < 1000 && sim::y < 200.0) {
    ++t;
    sim::step((unsigned long)t);
  }
  assert(isOnLine && !offlineEpisodeActive);

  // Kayip baglamini "keskin donus" yap: son konum kenarda, direksiyon buyuk.
  lastOnLinePosition = 14000;
  lastLinePWM = 400;
  lastDetectedSide = RIGHT;

  sim::lineEndsAtY = true;
  sim::lineEndY = sim::y - 1.0;

  const int tLoss = t + 1;
  sim::step((unsigned long)tLoss);
  assert(offlineEpisodeActive);
  assert(!offlineIsDash);           // kenar kaybi -> donus sinifi
  assert(offlineSearchSide == RIGHT);

  for (int i = 1; i <= 60; ++i) sim::step((unsigned long)(tLoss + i));

  const Rec *r = sim::at((unsigned long)(tLoss + OFFLINE_GAP_BRIDGE_MS + 10));
  assert(r && r->episode);
  printf("[T3] donus kaybi: tLoss=%d ms | +%d ms pwm=(%d,%d) beklenen=(%d,%d)\n",
         tLoss, OFFLINE_GAP_BRIDGE_MS + 10, r->pwmL, r->pwmR,
         OFFLINE_SEARCH_PWM, -OFFLINE_SEARCH_PWM);

  assert(r->pwmL == OFFLINE_SEARCH_PWM);   // sagda kaybettik -> saga donus
  assert(r->pwmR == -OFFLINE_SEARCH_PWM);
}

//  =====================================================================
//   T4 - Yabanci cizgi kapisi + turev darbesi olmadan yeniden yakalama
//  =====================================================================
void testForeignLineRejectedInDashWindow() {
  sim::reset(0.0, 10.0);
  sim::dashLenMm = 100000.0;
  sim::gapLenMm = 100000.0;
  beginRun();

  int t = 0;
  while (t < 1000 && sim::y < 200.0) {
    ++t;
    sim::step((unsigned long)t);
  }
  assert(isOnLine && !offlineEpisodeActive);

  // Kendi cizgimizi kes; ayni anda barin UCUNDAN gorunen yabanci bir cizgi
  // koy (pist disi zemin cizgisi / geri donus hatti benzetimi). Yabanci
  // cizgi 40 mm yanda -> olculen konum ~12500, yani merkez bandinin disi.
  sim::lineEndsAtY = true;
  sim::lineEndY = sim::y - 1.0;
  sim::foreignLine = true;
  sim::foreignLineLat = sim::x + 40.0;

  const int tLoss = t + 1;
  sim::step((unsigned long)tLoss);
  assert(offlineEpisodeActive && offlineIsDash);

  for (int i = 1; i <= 40; ++i) sim::step((unsigned long)(tLoss + i));

  const Rec *r = sim::at((unsigned long)(tLoss + 20));
  assert(r);
  printf("[T4] yabanci cizgi: t=%d ms onLine=%d kabul=%d pwm=(%d,%d)\n",
         (int)r->t, (int)r->onLine, (int)!r->episode, r->pwmL, r->pwmR);

  assert(r->onLine);                   // sensorler yabanci cizgiyi goruyor...
  assert(r->episode);                  // ...ama kabul edilmiyor (hala kopruda)
  assert(r->pwmL > 0 && r->pwmR > 0);  // duz devam, pivot yok

  // Simdi gercek devam cizgisi (merkezin 10 mm yaninda) geliyor.
  sim::foreignLine = false;
  sim::lineEndsAtY = false;
  sim::lineLatOffset = sim::x + 10.0;

  int t2 = tLoss + 20;
  while (t2 < tLoss + 300 && offlineEpisodeActive) {
    ++t2;
    sim::step((unsigned long)t2);
  }

  const Rec *r2 = sim::at((unsigned long)t2);
  assert(r2);
  const int steer = r2->pwmL - r2->pwmR;
  const int error = (int)getLinePosition() - LINE_CENTER_POSITION;
  printf("[T4] yeniden yakalama: t=%d ms kabul=%d pwm=(%d,%d) direksiyon=%d (turev darbesi olsa >200 olurdu)\n",
         (int)r2->t, (int)!r2->episode, r2->pwmL, r2->pwmR, steer);

  assert(!offlineEpisodeActive);  // cizgi kabul edildi
  assert(isOnLine);
  // Turev darbesi (derivative kick) kontrolu: previousError = error olmali.
  assert(previousError == error);
  assert(steer <= 210 && steer >= -210);  // turev darbesi olsaydi ~320 olurdu
}

//  =====================================================================
//   T5 - Cizgi tamamen biterse: yerinde arama + failsafe freni
//  =====================================================================
void testLineEndDoesNotRunaway() {
  sim::reset(0.0, 10.0);
  sim::dashLenMm = 100000.0;
  sim::gapLenMm = 100000.0;
  beginRun();

  int t = 0;
  while (t < 1000 && sim::y < 200.0) {
    ++t;
    sim::step((unsigned long)t);
  }

  sim::trackEnabled = false;  // cizgi bitti: bos zemin, yabanci cizgi yok

  const int tLoss = t + 1;
  sim::step((unsigned long)tLoss);
  assert(offlineEpisodeActive);

  int tStop = 0;
  for (int i = 1; i <= 1600; ++i) {
    sim::step((unsigned long)(tLoss + i));
    if (stopBrake_flag) {
      tStop = tLoss + i;
      break;
    }
  }
  assert(tStop > 0);

  const int failsafeMs = tStop - (int)offlineStartTime;
  const int worstX = sim::maxAbsX10(tLoss, tStop);
  const int worstYaw = sim::maxAbsYawDeg(tLoss, tStop);

  int maxPwm = 0;
  bool turnCcw = false, turnCw = false;
  for (int i = 0; i < sim::recordCount; ++i) {
    const Rec &r = sim::records[i];
    // Yalnizca ARAMA fazi (kopru penceresi disinda) incelenir: kopruda
    // PWM base +- holdSteer olabilir, sinir iddiasi arama icindir.
    if ((int)r.t < tLoss + OFFLINE_DASH_BRIDGE_MS || (int)r.t > tStop) continue;
    const int a = r.pwmL < 0 ? -r.pwmL : r.pwmL;
    const int b = r.pwmR < 0 ? -r.pwmR : r.pwmR;
    if (a > maxPwm) maxPwm = a;
    if (b > maxPwm) maxPwm = b;
    if (r.pwmL > 0 && r.pwmR < 0) turnCcw = true;
    if (r.pwmL < 0 && r.pwmR > 0) turnCw = true;
  }

  printf("[T5] cizgi bitisi: failsafe=%d ms (beklenen %d), max |x|=%d.%d mm, max |yaw|=%d derece, max |PWM|=%d, iki yon=%d\n",
         failsafeMs, OFFLINE_FAILSAFE_MS, worstX / 10, worstX % 10, worstYaw, maxPwm, (int)(turnCcw && turnCw));

  assert(failsafeMs >= OFFLINE_FAILSAFE_MS - 30 && failsafeMs <= OFFLINE_FAILSAFE_MS + 30);
  assert(worstX <= 600);            // 60 mm: yerinde dondu, pistten kacmadi
  assert(maxPwm <= OFFLINE_SEARCH_PWM);  // arama sinirli (eski surum: 200/-120)
  assert(turnCcw && turnCw);        // iki yonde de tarandi
}

//  =====================================================================
//   T6 - PD yon/isaret dogrulamasi: kacik baslangicta cizgiye oturur
//  =====================================================================
void testLineTrackingConverges() {
  sim::reset(25.0, 10.0);  // robot cizginin 25 mm yaninda basliyor
  sim::dashLenMm = 100000.0;
  sim::gapLenMm = 100000.0;
  beginRun();

  for (int t = 1; t <= 600; ++t) sim::step((unsigned long)t);

  const int finalX10 = sim::records[sim::recordCount - 1].x10;
  const int worstX10 = sim::maxAbsX10(0, 600);
  printf("[T6] yakinsama: baslangic x=25.0 mm | 600 ms sonra x=%d.%d mm | en buyuk sapma=%d.%d mm\n",
         finalX10 / 10, finalX10 % 10, worstX10 / 10, worstX10 % 10);

  assert(finalX10 >= -100 && finalX10 <= 100);  // 10 mm: cizgiye oturdu
  assert(worstX10 <= 800);                      // 80 mm: yon/isaret tutarli
  assert(isOnLine);
}

//  =====================================================================
//   M A I N
//  =====================================================================
int main() {
  printf("ATLAS antigravity-takim: kesikli cizgi (dash) davranis testi\n");
  printf("Model kabadir (PWM->hiz dogrusal); amac gercek .ino kontrol mantiginin regresyonu.\n\n");

  testStraightDashCrossing();
  testDashBridgeWindow();
  testCornerLossStillPivots();
  testForeignLineRejectedInDashWindow();
  testLineEndDoesNotRunaway();
  testLineTrackingConverges();

  printf("\nTum testler GECTI.\n");
  return 0;
}
