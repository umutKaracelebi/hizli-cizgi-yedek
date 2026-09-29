/*
  dash_gap_test.cpp - antigravity-takim "kesikli cizgi" (dash) davranis testi

  Amac
  ----
  Gercek firmware sekmelerini (gpt-takim-v1.2.ino, Motors.ino,
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
   5) START'tan sonra ONCE tribün mu devreye giriyor (28 Eyl)?
      (Beklenen: evet - ~1 sn on-vakum, tekerlekler komutsuz/uykuda; T9.)
   6) Viraj sonunda (duz cizgiye gecerken) robot GERIYE DONUYOR mu (28 Eyl)?
      (Beklenen: hayir - pivot yok, direksiyon yonu ters donmez; T10/T11.)

  Sinirlar
  --------
  Model kabadir: PWM -> hiz iliskisi dogrusal; enkoder/patinaj/vakum etkisi,
  motor zaman sabiti ve gercek gecikmeler yoktur. Sonuc "gercek pistte
  dogrulandi" anlamina gelmez; kontrol mantiginin regresyon testidir.

  Derleme ve kosum (tests klasorunde; g++ veya clang++ gerekir)
  --------------------------------------------------------------
    g++ -std=c++11 -Wall -Wextra -pedantic -o gpt_takim_v1_2_test.exe dash_gap_test.cpp
*/

#include "stubs/Arduino.h"

#include "atlas_prototypes.h"

// Arduino derleyicisiyle ayni sira: ana sekme, sonra diger sekmeler (alfabetik).
#include "../gpt-takim-v1.2.ino"
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

bool fullBlackFloor = false;  // H2 testi: butun zemin siyah (pist disi / robot havada)

bool sensorMaskOverride = false;  // testte belirsiz kesişim çıkışını birebir kurar
uint16_t sensorMask = 0;

bool hasBlackAtLat(double lat, double lon) {
  if (fullBlackFloor) return true;
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
  fullBlackFloor = false;
  sensorMaskOverride = false;
  sensorMask = 0;
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
    const bool black = sensorMaskOverride
                           ? ((sensorMask & ((uint16_t)1 << i)) != 0)
                           : sensorSeesBlack(i);
    TestBoard::adcRaw[i] = black ? 0 : 255;  // siyah = 0 (ters mod)
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

  // Kopru sirasinda (episode kayitlari) robot DONMEMELI: pwm esit, yaw sabit
  int maxCoastYawDelta = 0;
  {
    bool inEp = false;
    int epStartYaw = 0;
    for (int i = 0; i < sim::recordCount; ++i) {
      const Rec &r = sim::records[i];
      if (r.episode && !inEp) { inEp = true; epStartYaw = r.yawDeg; }
      else if (!r.episode) { inEp = false; }
      else {
        int d = r.yawDeg - epStartYaw;
        if (d < 0) d = -d;
        if (d > maxCoastYawDelta) maxCoastYawDelta = d;
      }
    }
  }

  // Yeniden yakalama yumusatmasi: episode kapandiktan sonraki DAMP penceresi
  // boyunca direksiyon REACQUIRE_STEER_CLAMP'i asmamali.
  int maxDampSteer = 0;
  {
    for (int i = 1; i < sim::recordCount; ++i) {
      const Rec &r = sim::records[i];
      const Rec &p = sim::records[i - 1];
      if (p.episode && !r.episode) {  // kabul ani
        const unsigned long winEnd = r.t + REACQUIRE_DAMP_MS;
        for (int j = i; j < sim::recordCount && sim::records[j].t < winEnd; ++j) {
          if (sim::records[j].episode) break;  // yeni kayip: ornek bitti
          int steer = sim::records[j].pwmL - sim::records[j].pwmR;
          if (steer < 0) steer = -steer;
          if (steer > maxDampSteer) maxDampSteer = steer;
        }
      }
    }
  }

  printf("[T1] t=%d ms, beyaz bosluk=%d, kayip dongusu=%d (dash=%d), negatif PWM dongusu=%d, koprude max direksiyon=%d\n",
         t, gaps, episodeRecords, dashRecords, negativePwmRecords, maxSteer);
  printf("[T1] max |x|=%d.%d mm, kopru icinde max yaw degisimi=%d derece, damp penceresi max direksiyon=%d\n",
         worstX / 10, worstX % 10, maxCoastYawDelta, maxDampSteer);

  assert(settled);                        // bosluklar gecildi, cizgi yeniden bulundu
  assert(gaps >= 3);                      // kesikler gercekten deneyimlendi
  assert(dashRecords == episodeRecords);  // hepsi "duz kesik" olarak siniflandi
  assert(negativePwmRecords == 0);        // boslukta pivot/donus YOK (duz devam)
  assert(maxSteer == 0);                  // koprude direksiyon tamamen sifir
  assert(maxCoastYawDelta <= 3);          // bosluk sirasinda donus yok
  assert(maxDampSteer <= 2 * REACQUIRE_STEER_CLAMP);  // yumusatma aktif
  assert(worstX <= 400);                  // 40 mm: seritten cikilmadi
}

//  =====================================================================
//   T2 - Kopru penceresi: kopru boyunca TAM duz, dolduktan sonra guvenli durus
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

  for (int i = 1; i <= (int)OFFLINE_DASH_COAST_MS + 20; ++i) sim::step((unsigned long)(tLoss + i));

  const Rec *early = sim::at((unsigned long)(tLoss + OFFLINE_GAP_BRIDGE_MS));
  const Rec *mid = sim::at((unsigned long)(tLoss + OFFLINE_DASH_COAST_MS / 2));
  const Rec *late = sim::at((unsigned long)(tLoss + OFFLINE_DASH_COAST_MS + 5));
  assert(early && mid && late);

  printf("[T2] kopru: tLoss=%d ms | +%d ms pwm=(%d,%d) | +%d ms pwm=(%d,%d) | +%d ms (kopru dolmus) pwm=(%d,%d)\n",
         tLoss, OFFLINE_GAP_BRIDGE_MS, early->pwmL, early->pwmR,
         OFFLINE_DASH_COAST_MS / 2, mid->pwmL, mid->pwmR,
         OFFLINE_DASH_COAST_MS + 5, late->pwmL, late->pwmR);

  // Kopru boyunca robot TAM DUZ gider (direksiyon sifir, iki teker esit ileri).
  assert(early->pwmL > 0 && early->pwmR > 0);
  assert(mid->pwmL > 0 && mid->pwmR > 0);
  assert(early->pwmL == early->pwmR);
  assert(mid->pwmL == mid->pwmR);
  assert(early->yawDeg <= 3 && early->yawDeg >= -3);  // donus baslamadi
  assert(early->episode && mid->episode && late->episode);

  // Kesik koprusu dolduktan sonra ARAMA YOK: dogrudan guvenli fren baglar.
  // Beyaz alanda yerinde donus (geri kilitlenme arizasi) engellenir.
  assert(late->pwmL == late->pwmR);  // simetrik fren rampasi, pivot yok
  assert(stopBrake_flag);
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

  // Kayip baglaminin "keskin donus" olmasini dayat: sonulmus konum kenarda,
  // sonulmus direksiyon buyuk. Cizgi TAMAMEN kaldirilir ki yerinde donus
  // sirasinda "arkadan gorunme" koreografiyi bozmasin (gercek 90 derece
  // kosede eski duz parca artik gorunmez; onu T3b modeller).
  posSmooth = 14000;
  steerSmooth = 400;
  lastDetectedSide = RIGHT;
  sim::trackEnabled = false;

  const int tLoss = t + 1;
  sim::step((unsigned long)tLoss);
  assert(offlineEpisodeActive);
  assert(!offlineIsDash);           // kenar kaybi -> donus sinifi
  assert(offlineSearchSide == RIGHT);

  // Butcenin sonuna kadar (biraz gecerek) kostur
  const unsigned long tSearchEnd = tLoss + OFFLINE_GAP_BRIDGE_MS + OFFLINE_SEARCH_MAX_MS;
  for (int i = 1; i <= (int)(tSearchEnd - tLoss) + 10; ++i) {
    sim::step((unsigned long)(tLoss + i));
  }

  const Rec *r0 = sim::at((unsigned long)(tLoss + OFFLINE_GAP_BRIDGE_MS + 10));
  const Rec *s1 = sim::at((unsigned long)(tLoss + OFFLINE_GAP_BRIDGE_MS + OFFLINE_SEARCH_FIRST_MS + 20));
  const Rec *s2 = sim::at((unsigned long)(tSearchEnd + 5));
  assert(r0 && s1 && s2);
  printf("[T3] donus kaybi: tLoss=%d ms | ilk faz pwm=(%d,%d) | ters faz pwm=(%d,%d) | butce sonu pwm=(%d,%d) fren=%d\n",
         tLoss, r0->pwmL, r0->pwmR, s1->pwmL, s1->pwmR, s2->pwmL, s2->pwmR, (int)stopBrake_flag);

  assert(r0->pwmL == OFFLINE_SEARCH_PWM);    // sagda kaybettik -> once saga
  assert(r0->pwmR == -OFFLINE_SEARCH_PWM);
  assert(s1->pwmL == -OFFLINE_SEARCH_PWM);   // sonra TEK ters faz
  assert(s1->pwmR == OFFLINE_SEARCH_PWM);
  assert(stopBrake_flag);                    // butce doldu -> fren (ASLA geri donerek kilitlenmez)
  // H1 sifati: fren pivot anindaki cikisara mandallanir. Burada tetik
  // ters fazdayken geldi: (L=-120, R=+120). Eski surum burada TAM GAZ
  // ileri (velocityPWM, velocityPWM) sicrardi — bu test onu yakalar.
  assert(s2->pwmL < 0 && s2->pwmR > 0);      // pivot yonunde frenliyor (ileri sicrama YOK)
  assert(s2->pwmL >= -OFFLINE_SEARCH_PWM && s2->pwmR <= OFFLINE_SEARCH_PWM);  // mandal sinirinda
}

//  =====================================================================
//   T3b - Donus sirasinda gorunen cizgi: once GECICI TAKIP, kesintisiz
//         surerse kabul; arkadan kisa temas kilitlenemez
//  =====================================================================
void testCornerSearchProvisionalTrack() {
  sim::reset(0.0, 10.0);
  sim::dashLenMm = 100000.0;
  sim::gapLenMm = 100000.0;
  beginRun();

  int t = 0;
  while (t < 1000 && sim::y < 200.0) {
    ++t;
    sim::step((unsigned long)t);
  }

  // Donus sinifi dayat; kendi cizgimizi kaldiriyoruz ve DONUS YONUNDE,
  // barin erisimine yakin (50 mm sagda) gercek bir devam parcasi koyuyoruz.
  // Arama ona yaklastiginda once GECICI TAKIP (kisitli direksiyon) baslamali,
  // temas TRACK_MS kesintisiz surunce kabul edilmeli.
  posSmooth = 14000;
  steerSmooth = 400;
  lastDetectedSide = RIGHT;
  sim::trackEnabled = false;

  const int tLoss = t + 1;
  sim::step((unsigned long)tLoss);
  assert(offlineEpisodeActive && !offlineIsDash);

  // Devam parcasi ancak KAYIP SINIFLANDIRILDIKTAN sonra sahneye girer
  // (aksi halde ilk karede episode yokken normal cizgi gibi kabul edilirdi).
  sim::foreignLine = true;
  sim::foreignLineLat = sim::x + 50.0;  // saga donuste bar ucunun erisecegi mesafe

  // Donus/takip suresince gorunen hicbir karede TAM PID hamlesi olmamali:
  // ya arama pivotu (+-SEARCH) ya da kisitli gecici takip (|steer|<=CLAMP).
  bool sawProvisional = false;
  int tAccepted = -1;
  int tEnd = tLoss + OFFLINE_GAP_BRIDGE_MS + OFFLINE_SEARCH_MAX_MS + (int)OFFLINE_SEARCH_TRACK_MS + 300;
  for (int i = 1; i <= tEnd - tLoss; ++i) {
    sim::step((unsigned long)(tLoss + i));
    const Rec &r = sim::records[sim::recordCount - 1];
    const bool spinning = (r.pwmL == -OFFLINE_SEARCH_PWM && r.pwmR == OFFLINE_SEARCH_PWM) ||
                          (r.pwmL == OFFLINE_SEARCH_PWM && r.pwmR == -OFFLINE_SEARCH_PWM);
    if (!spinning && r.episode && !stopBrake_flag) {
      // Gecici takip ya da kisa kopru: kisitli olmali
      int steer = r.pwmL - r.pwmR;
      if (steer < 0) steer = -steer;
      assert(steer <= 2 * REACQUIRE_STEER_CLAMP);
      if (r.onLine) sawProvisional = true;
    }
    if (tAccepted < 0 && !r.episode) tAccepted = r.t;
  }

  // Fren tetiklendiyse tamamlanana kadar (500 ms rampa) kostur.
  // (Not: testler run() yerine runInit/updatePeriod cagirir; isRunning
  // bayragi burada anlam tasimaz, sureye bakilir.)
  if (stopBrake_flag) {
    const unsigned long tBrakeEnd = stopBrakeStartTime + STOP_BRAKE_TIME_MS + 5;
    while (tEnd < (int)tBrakeEnd) {
      ++tEnd;
      sim::step((unsigned long)tEnd);
    }
  }

  const int finalX10 = sim::records[sim::recordCount - 1].x10;
  printf("[T3b] tLoss=%d, gecici takip goruldu=%d, kabul=%s, fren=%d, son x=%d.%d mm (devam ~50 mm sagda)\n",
         tLoss, (int)sawProvisional, tAccepted > 0 ? "var" : "yok",
         (int)stopBrake_flag, finalX10 / 10, finalX10 % 10);

  // Sozlesme: kalici devam cizgisi gecici takip sonrasi KABUL edildi;
  // arama hicbir zaman sonsuz pivota gitmedi; en sonda ya cizgide veya
  // guvenli fren ile DURMUS olmali. Son konum kaba modelde fren anina
  // bagli degiskendir — sinirli bolgede kalmak yeterlidir.
  if (tAccepted > 0) {
    assert(sawProvisional);                 // kabulden once kisitli takip calisti
    if (!stopBrake_flag) assert(isOnLine);  // frenlenmemisse hala cizgide
  }
  if (stopBrake_flag) {
    const Rec &last = sim::records[sim::recordCount - 1];
    assert(last.pwmL == 0 && last.pwmR == 0);   // fren tamamlandi, kilit
  }
  assert(finalX10 >= -1500 && finalX10 <= 1500);  // 150 mm: pist bolgesinden kacmadi
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

  // Kendi cizgimizi TEK basina kes: kesik sinifi "duz" cikmali.
  // (Yabanci cizgi ancak kopru BASLADIKTAN sonra sahneye girer; aksi
  // halde ilk karede henuz episode yokken normal cizgi gibi kabul
  // edilirdi — bu senaryonun eski tasarim hatasiydi.)
  sim::lineEndsAtY = true;
  sim::lineEndY = sim::y - 1.0;

  const int tLoss = t + 1;
  sim::step((unsigned long)tLoss);
  assert(offlineEpisodeActive && offlineIsDash);

  // Kesik koprusunun ortasinda, barin UCUNDAN gorunen yabanci bir cizgi
  // beliriyor (pist disi zemin cizgisi benzetimi): 40 mm yanda ->
  // konum ~12500, merkez bandinin DISI. SIDE_CONFIRM (40 ms) dolmadan
  // kaybolacak: 30 ms gosterilir.
  sim::foreignLine = true;
  sim::foreignLineLat = sim::x + 40.0;

  for (int i = 1; i <= 30; ++i) sim::step((unsigned long)(tLoss + i));

  const Rec *r = sim::at((unsigned long)(tLoss + 25));
  assert(r);
  printf("[T4] yabanci cizgi: t=%d ms onLine=%d kabul=%d pwm=(%d,%d)\n",
         (int)r->t, (int)r->onLine, (int)!r->episode, r->pwmL, r->pwmR);

  assert(r->onLine);                   // sensorler yabanci cizgiyi goruyor...
  assert(r->episode);                  // ...ama kabul edilmiyor (hala kopruda)
  assert(r->pwmL > 0 && r->pwmR > 0);  // duz devam, pivot yok
  assert(r->pwmL == r->pwmR);          // TAM duz (direksiyon sifir)

  // Yabanci cizgi kalkiyor; gercek devam cizgisi (merkezin 10 mm
  // yaninda) geliyor.
  sim::foreignLine = false;
  sim::lineEndsAtY = false;
  sim::lineLatOffset = sim::x + 10.0;

  int t2 = tLoss + 30;
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
//   T5 - Cizgi tamamen biterse (merkezdeyken): DUZ kopru -> guvenli durus
//        (kesik sinifi: asla yerinde donus olmamali)
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
  assert(offlineIsDash);  // merkezde kayip -> "uzun kesik" sinifi

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

  int maxSteerCoast = 0;
  bool anyNegative = false;
  for (int i = 0; i < sim::recordCount; ++i) {
    const Rec &r = sim::records[i];
    if ((int)r.t < tLoss || (int)r.t > tStop) continue;
    const int steer = r.pwmL - r.pwmR;
    const int a = steer < 0 ? -steer : steer;
    if (a > maxSteerCoast) maxSteerCoast = a;
    if (r.pwmL < 0 || r.pwmR < 0) anyNegative = true;
  }

  printf("[T5] cizgi bitisi (kesik sinifi): durus=%d ms (beklenen %d), max |x|=%d.%d mm, max |yaw|=%d derece, kopru max direksiyon=%d, negatifPWM=%d\n",
         failsafeMs, OFFLINE_DASH_COAST_MS, worstX / 10, worstX % 10, worstYaw, maxSteerCoast, (int)anyNegative);

  assert(failsafeMs >= OFFLINE_DASH_COAST_MS - 30 && failsafeMs <= OFFLINE_DASH_COAST_MS + 30);
  assert(maxSteerCoast == 0);         // tek dongu bile direksiyon yok: TAM duz
  assert(!anyNegative);               // yerinde donus/pivot YOK (geri kilitlenme engeli)
  assert(worstYaw <= 15);             // donus olmadi
  assert(worstX <= 600);              // 60 mm: duz devam edip durdu, kacmadi
}

//  =====================================================================
//   T6 - PD yon/isaret dogrulamasi: kacik baslangicta cizgiye DONER ve
//        seritte sinirli kalir
//  =====================================================================
//  NOT: Bu kaba modelde sensorler ikilidir (0/255); agirlikli ortalama
//  ~500 birimlik adimlarla siçrar ve D terimi bu adimlarda buyuk darbe
//  uretir. Gercek kartta analog degrade oldugundan salinimin genligi
//  cok daha kucuktur. Bu yuzden "+-10 mm'ye eksiksiz oturma" bu modelde
//  fiziksel olarak beklenemez; sozlesme: dogru yone donus, merkeze
//  yaklasma ve sinirlilik.
void testLineTrackingConverges() {
  sim::reset(25.0, 10.0);  // robot cizginin 25 mm yaninda basliyor
  sim::dashLenMm = 100000.0;
  sim::gapLenMm = 100000.0;
  beginRun();

  int minX10 = 250;  // ilk 300 ms icinde merkeze en yakin nokta
  for (int t = 1; t <= 600; ++t) {
    sim::step((unsigned long)t);
    if (t >= 40 && t <= 300) {
      int ax = sim::records[sim::recordCount - 1].x10;
      if (ax < 0) ax = -ax;
      if (ax < minX10) minX10 = ax;
    }
  }

  const int finalX10 = sim::records[sim::recordCount - 1].x10;
  const int worstX10 = sim::maxAbsX10(0, 600);
  const int worstXPreBrake = sim::maxAbsX10(0, 550);  // fren oncesi islemsel pencere
  printf("[T6] yon/oturma: baslangic x=25.0 mm | 600 ms sonra x=%d.%d mm | ilk 300 ms en yakin=%d.%d mm | fren oncesi en buyuk sapma=%d.%d mm\n",
         finalX10 / 10, finalX10 % 10, minX10 / 10, minX10 % 10, worstXPreBrake / 10, worstXPreBrake % 10);

  assert(minX10 <= 150);          // 15 mm: merkeze dogru dondugu KANIT (isaret dogru)
  assert(worstX10 <= 800);        // 80 mm: modeldeki kaba limit-cycle sinirli kaldi
  (void)finalX10;                 // son konum: kaba modelde limit-cycle fazina bagli,
                                  // fren sonrasi pistten nerede durdugunu olcmez
}

//  =====================================================================
//   T7 - H2 kaksi: TAM SIYAH zemin (pist disi / robot havada) artik
//        sonsuz "kesisim" sayilmiyor; MAX suresinde guvenli durusa gider
//  =====================================================================
void testFullBlackFloorStops() {
  sim::reset(0.0, 10.0);
  sim::dashLenMm = 100000.0;
  sim::gapLenMm = 100000.0;
  beginRun();

  int t = 0;
  while (t < 1000 && sim::y < 150.0) {  // rampayken zaten cizgi goruluyor
    ++t;
    sim::step((unsigned long)t);
  }
  assert(isOnLine && !inIntersection);

  // Bir andan itibaren BUTUN zemin siyah (araba pist disi zemin / havada
  // kaldirilmis benzetimi). Eski kod burada sonsuza dek duz surerdi.
  const int tBlack = t;
  sim::fullBlackFloor = true;

  int tBrake = -1;
  for (int i = 1; i <= 1200; ++i) {
    sim::step((unsigned long)(tBlack + i));
    if (stopBrake_flag && tBrake < 0) tBrake = tBlack + i;
  }

  printf("[T7] tam siyah: giris t=%d, fren baslangici=%s (beklenen ~t+%d)\n",
         tBlack, tBrake > 0 ? "var" : "YOK", INTERSECTION_MAX_MS);

  assert(tBrake > 0);                                        // guvenli durus TETIKLENDI
  assert(tBrake - tBlack >= INTERSECTION_MAX_MS - 5);        // gercek kesisimler bundan once durmaz
  assert(tBrake - tBlack <= INTERSECTION_MAX_MS + 60);       // hold payiyla birlikte tavanda
  // Fren basladiktan sonra motor sonunda sifirlanir (failsafe kilit)
  const Rec &last = sim::records[sim::recordCount - 1];
  assert(last.pwmL == 0 && last.pwmR == 0);
}

//  =====================================================================
//   T8 - H1 kaksi: hiz rampasi ORTASINDA failsafe teterse fren
//        velocityPWM'den degil o anki dusuk rampadan baslar (sicrama yok)
//  =====================================================================
void testBrakeLatchDuringRamp() {
  sim::reset(0.0, 10.0);
  sim::dashLenMm = 100000.0;
  sim::gapLenMm = 100000.0;
  beginRun();

  // Hiz rampasi 150 ms; 60 ms noktasinda robot henuz ~%40 hizda gidiyor.
  int t = 0;
  while (t < 60) { ++t; sim::step((unsigned long)t); }
  const int latchL = outputPWML;   // beklenen mandal degeri
  const int latchR = outputPWMR;
  assert(latchL > 0 && latchL < velocityPWM);   // rampa ortasi: tam gazda DEGIL

  startBrake();  // failsafe/STOP tetiklenmesiyle ayni merkezi yol

  for (int i = 1; i <= 600; ++i) sim::step((unsigned long)(t + i));

  const Rec *firstBrake = sim::at((unsigned long)(t + 1));
  const Rec &last = sim::records[sim::recordCount - 1];
  printf("[T8] rampada fren: mandal=(%d,%d) | ilk fren pwm=(%d,%d) | son pwm=(%d,%d) | velocityPWM=%d\n",
         latchL, latchR, firstBrake ? firstBrake->pwmL : -999,
         firstBrake ? firstBrake->pwmR : -999, last.pwmL, last.pwmR, velocityPWM);

  assert(firstBrake);
  assert(firstBrake->pwmL >= 0 && firstBrake->pwmR >= 0);   // geri firlatma yok
  // Mandal sinirinda baslar (1 ms geçiş + int yuvarlama payi)
  assert(firstBrake->pwmL <= latchL + 2 && firstBrake->pwmR <= latchR + 2);
  assert(firstBrake->pwmL < velocityPWM);                   // TAM GAZA sicramadi (H1)
  assert(last.pwmL == 0 && last.pwmR == 0);                 // fren sonunda durdu
}

//  =====================================================================
//   T9 - ON-VAKUM (28 Eyl): START'tan sonra ~1 sn tribün TAM DEVIRDE,
//        tekerlekler HAREKETSIZ (suruculer uykuda) ve RUN'da tribün
//        yeniden rampa YAPMAZ.
//  =====================================================================
void testPreVacuum() {
  // --- A) Tam on-vakum -------------------------------------------------
  TestBoard::nowUs = 0;
  PIND &= ~(1 << PIND4);  // MEBSTART = LOW (START)
  outputPWML = 0;
  outputPWMR = 0;
  OCR1A = 0;
  OCR1B = 0;
  TestBoard::motorDriverEnabled = false;

  const unsigned long t0 = millis();
  const bool completed = preVacuum();
  const unsigned long vacMs = millis() - t0;

  printf("[T9] on-vakum: tamam=%d sure=%lu ms (beklenen >=%d) | tribun OCR2A=%d (hedef %d) | tekerlek OCR1A=%d OCR1B=%d | surucu=%d\n",
         (int)completed, vacMs, PRE_VACUUM_TIME_MS, (int)OCR2A, IMPELLER_PWM,
         (int)OCR1A, (int)OCR1B, (int)TestBoard::motorDriverEnabled);

  assert(completed);                                        // on-vakum tamamlandi
  assert(vacMs >= PRE_VACUUM_TIME_MS);                      // en az 1 sn calisti
  assert(vacMs <= PRE_VACUUM_TIME_MS + 40);                 // 2 ms adimli dongu payi
  assert(PRE_VACUUM_TIME_MS >= RUN_IMPELLER_RAMP_TIME_MS);  // ramp pencereye sigar
  assert(OCR2A == IMPELLER_PWM);                            // tribün TAM DEVIRDE
  assert(impPWMDecrement == 0);                             // RUN'da yeniden rampa yok
  assert(outputPWMImp == IMPELLER_PWM);
  assert(!TestBoard::motorDriverEnabled);                   // suruculer HALA uykuda
  assert(OCR1A == 0 && OCR1B == 0);                         // tekerlek PWM'i uretilmedi
  assert(outputPWML == 0 && outputPWMR == 0);               // tekerlek komutu verilmedi

  // --- B) RUN girisi: on-vakumdan devralma, kosu boyunca sabit tribün -
  TestBoard::nowUs = 0;
  runBegin();
  assert(TestBoard::motorDriverEnabled);                    // tekerlek suruculeri uyandi
  assert(impPWMDecrement == 0);
  assert(outputPWMImp == IMPELLER_PWM);

  sim::reset(0.0, 10.0);
  sim::dashLenMm = 100000.0;  // tek parca cizgi (kayip yok)
  sim::gapLenMm = 100000.0;
  sim::recordCount = 0;
  setupCalibration();
  velocityPWM = BASE_VELOCITY_PWM;

  bool impellerHeld = true;
  for (int t = 1; t <= 400; ++t) {
    sim::step((unsigned long)t);
    if (outputPWMImp != IMPELLER_PWM) impellerHeld = false;
  }
  printf("[T9] RUN: 400 dongu boyunca tribun sabit=%d (OCR2A=%d, outputPWMImp=%d)\n",
         (int)impellerHeld, (int)OCR2A, outputPWMImp);
  assert(impellerHeld);                                     // kosuda asla sifirlanmaz

  // --- C) Erken STOP: on-vakum iptal, tribün kapanir -------------------
  TestBoard::nowUs = 0;
  PIND |= (1 << PIND4);  // MEBSTART = HIGH (STOP)
  const unsigned long tCancel0 = millis();
  const bool cancelled = preVacuum();
  const unsigned long cancelMs = millis() - tCancel0;

  printf("[T9] erken STOP: tamam=%d (beklenen 0) sure=%lu ms | tribun OCR2A=%d\n",
         (int)cancelled, cancelMs, (int)OCR2A);

  assert(!cancelled);                                       // iptal edildi
  assert(cancelMs >= STOP_SIGNAL_CONFIRM_MS);               // STOP teyidi kadar bekledi
  assert(cancelMs < PRE_VACUUM_TIME_MS);                    // 1 sn dolmadan bitti
  assert(OCR2A == 0);                                       // tribün kapandi
}

//  =====================================================================
//   T10 - VIRAJ CIKISI: cizgi barin UCUNDA gorunuyor (konum 15000).
//         27 Eyl kodu bunu "keskin kose" sayip yerinde donuslu arama
//         baslatiyordu: robot viraj sonunda GERIYE DONUYOR / DISA ATIYORDU.
//         Sozlesme: pivot yok, duzeltme yonu TERS DONMEZ, temas TRACK_MS
//         icinde kabul edilir, robot seritten kacmaz.
//  =====================================================================
void testCurveExitEdgeContactDoesNotReverse() {
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

  // VIRAJ CIKISI benzetimi: robot virajdan egik cikar -> cizgi barin
  // UCUNDA kalir. Kayip baglami da egridir (sonulmus konum kenarda).
  //  - Cizgi 65 mm sagda: barin YALNIZCA en uc sensoru (15) gorur =>
  //    konum 15000, isOnLine true ama lineFound false.
  //  - posSmooth = 13000: merkez bandinin DISINDA (egri cikisi baglami).
  sim::lineLatOffset = sim::x + 65.0;
  posSmooth = 13000;
  steerSmooth = 300;

  const int tLoss = t + 1;
  sim::step((unsigned long)tLoss);
  assert(offlineEpisodeActive);
  assert(!offlineIsDash);      // merkez disi kayip
  assert(!offlineIsEdgeLoss);  // ...ama bar cizgiyi GORUYOR -> pivot YOK
  assert(!stopBrake_flag);     // fren yok

  int tAccepted = -1;
  bool sawPivot = false;
  int flips = 0;
  int prevSign = 0;
  int maxSteer = 0;
  int minPwm = 100000;
  int maxX10 = 0;
  const int tEnd = tLoss + (int)OFFLINE_FAILSAFE_MS + 200;

  for (int i = 1; i <= tEnd - tLoss; ++i) {
    sim::step((unsigned long)(tLoss + i));
    const Rec &r = sim::records[sim::recordCount - 1];
    if (!r.episode) {
      tAccepted = (int)r.t;
      break;
    }
    const int steer = r.pwmL - r.pwmR;
    const int absSteer = steer < 0 ? -steer : steer;
    if (absSteer > maxSteer) maxSteer = absSteer;
    if (r.pwmL == OFFLINE_SEARCH_PWM && r.pwmR == -OFFLINE_SEARCH_PWM) sawPivot = true;
    if (r.pwmL == -OFFLINE_SEARCH_PWM && r.pwmR == OFFLINE_SEARCH_PWM) sawPivot = true;
    const int sign = steer > 0 ? 1 : (steer < 0 ? -1 : 0);
    if (sign != 0) {
      if (prevSign != 0 && sign != prevSign) flips++;
      prevSign = sign;
    }
    const int low = r.pwmL < r.pwmR ? r.pwmL : r.pwmR;
    if (low < minPwm) minPwm = low;
    const int ax = r.x10 < 0 ? -r.x10 : r.x10;
    if (ax > maxX10) maxX10 = ax;
  }

  const int acceptMs = tAccepted > 0 ? tAccepted - tLoss : -1;
  printf("[T10] viraj cikisi (kenar temasi): tLoss=%d | pivot=%d | yon ters donme=%d | kabul=%d ms (TRACK=%d) | max direksiyon=%d | en dusuk teker PWM=%d | max |x|=%d.%d mm\n",
         tLoss, (int)sawPivot, flips, acceptMs, OFFLINE_SEARCH_TRACK_MS,
         maxSteer, minPwm, maxX10 / 10, maxX10 % 10);

  assert(tAccepted > 0);                              // temas kabul edildi
  assert(!sawPivot);                                  // yerinde donus YOK
  assert(flips == 0);                                 // geriye donme YOK
  assert(acceptMs <= 2 * OFFLINE_SEARCH_TRACK_MS);     // kesintili temas payiyla hizli kabul
                                                       // (pivot aramasi 280 ms surer + ters donerdi)
  assert(!stopBrake_flag && isOnLine);                // cizgiye donuldu
  assert(maxSteer <= 2 * REACQUIRE_STEER_CLAMP);      // hamle sinirli
  assert(minPwm >= 0);                                // ic teker geri donmedi
  assert(maxX10 <= 900);                              // 90 mm: seritten kacmadi
}

//  =====================================================================
//   T11 - VIRAJ CIKISI (cizgi tamamen gorunmuyor; konum merkez bandi
//         DISINDA ama bar ucunda DEGIL = egri): KOR VIRAJ TAKIBI.
//         Sozlesme: pivot / ters donus YOK, direksiyon SONER, iki teker
//         de ileri kalir, butce dolunca guvenli durus.
//  =====================================================================
void testCurveExitBlindGuidedCoast() {
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

  // Kayip baglamini "viraj" olarak dayat: sonulmus konum merkez bandinin
  // disinda (egri) ama bar ucunda DEGIL; cizgi tamamen kalkar.
  posSmooth = 11800;
  steerSmooth = 400;
  lastDetectedSide = RIGHT;
  sim::trackEnabled = false;

  const int tLoss = t + 1;
  sim::step((unsigned long)tLoss);
  assert(offlineEpisodeActive);
  assert(!offlineIsDash);      // duz kesik degil
  assert(!offlineIsEdgeLoss);  // bar ucunda degil -> pivot YOK

  bool sawPivot = false;
  int flips = 0;
  int prevSign = 0;
  int maxSteer = 0;
  int firstSteer = -1;
  int lastSteer = 0;
  int minPwm = 100000;
  int tBrake = -1;

  const int tEnd = tLoss + (int)OFFLINE_FAILSAFE_MS + 200;
  for (int i = 1; i <= tEnd - tLoss; ++i) {
    sim::step((unsigned long)(tLoss + i));
    const Rec &r = sim::records[sim::recordCount - 1];
    if (r.pwmL == OFFLINE_SEARCH_PWM && r.pwmR == -OFFLINE_SEARCH_PWM) sawPivot = true;
    if (r.pwmL == -OFFLINE_SEARCH_PWM && r.pwmR == OFFLINE_SEARCH_PWM) sawPivot = true;
    if (r.episode && !stopBrake_flag) {
      const int steer = r.pwmL - r.pwmR;
      const int absSteer = steer < 0 ? -steer : steer;
      if (absSteer > maxSteer) maxSteer = absSteer;
      // Sonme kaniti: kor viraj takibinin BASI ile SONU karsilastirilir.
      if (r.t - (unsigned long)tLoss > OFFLINE_GAP_BRIDGE_MS) {
        if (firstSteer < 0) firstSteer = absSteer;
        lastSteer = absSteer;
      }
      const int sign = steer > 0 ? 1 : (steer < 0 ? -1 : 0);
      if (sign != 0) {
        if (prevSign != 0 && sign != prevSign) flips++;
        prevSign = sign;
      }
      const int low = r.pwmL < r.pwmR ? r.pwmL : r.pwmR;
      if (low < minPwm) minPwm = low;
    }
    if (stopBrake_flag && tBrake < 0) tBrake = (int)r.t;
  }

  const int brakeMs = tBrake > 0 ? tBrake - tLoss : -1;
  printf("[T11] kor viraj takibi: tLoss=%d | pivot=%d | yon ters donme=%d | max direksiyon=%d (ilk %d -> son %d) | en dusuk teker PWM=%d | fren=%d ms (butce %d)\n",
         tLoss, (int)sawPivot, flips, maxSteer, firstSteer, lastSteer, minPwm, brakeMs,
         OFFLINE_GUIDED_BUDGET_MS);

  assert(!sawPivot);                                   // yerinde donus YOK
  assert(flips == 0);                                  // geriye donme YOK
  assert(brakeMs > 0);                                 // butce dolunca guvenli durus
  assert(brakeMs >= OFFLINE_GUIDED_BUDGET_MS - 5);
  assert(brakeMs <= OFFLINE_GUIDED_BUDGET_MS + 30);
  assert(maxSteer <= 2 * OFFLINE_GUIDED_STEER_MAX);    // yumusak direksiyon
  assert(firstSteer > 0);                              // kor takip direksiyonu uygulandi
  assert(lastSteer < firstSteer);                      // ...ve SONDÜ (yay gibi tutmaz)
  assert(minPwm > 0);                                  // iki teker de ILERI
}

//  =====================================================================
//   T12 - Loop kesişiminden yalnızca ileri hat dar/merkezde teyitliyse
//         çıkılır; yanal kol PID'yi erken açamaz.
//  =====================================================================
void testIntersectionPassesStraightBeforePid() {
  sim::reset(0.0, 10.0);
  sim::dashLenMm = 100000.0;
  sim::gapLenMm = 100000.0;
  beginRun();

  const uint16_t centerMask = ((uint16_t)1 << 7) | ((uint16_t)1 << 8);
  const uint16_t intersectionMask = 0xFFFF;
  const uint16_t sideLegMask = ((uint16_t)1 << 10) | ((uint16_t)1 << 11);
  unsigned long t = 0;

  sim::sensorMaskOverride = true;

  for (int i = 0; i < 30; ++i) {
    sim::sensorMask = centerMask;
    sim::step(++t);
  }

  // Kesişim altındayken tüm bar siyah: çıkışlar eşit olmalı.
  for (int i = 0; i < 30; ++i) {
    sim::sensorMask = intersectionMask;
    sim::step(++t);
    assert(inIntersection);
    assert(outputPWML == outputPWMR);
    assert(!stopBrake_flag);
  }

  // Yanal kol artık daralsa bile merkez/ileri hat değil. Eski 50 ms hold
  // burada PID'yi açıp büyük D darbesiyle bu kola döndürüyordu.
  for (int i = 0; i < 100; ++i) {
    sim::sensorMask = sideLegMask;
    sim::step(++t);
    assert(inIntersection);
    assert(outputPWML == outputPWMR);
    assert(!stopBrake_flag);
  }

  bool pidResumed = false;
  for (int i = 0; i < INTERSECTION_EXIT_CONFIRM_MS + 5; ++i) {
    sim::sensorMask = centerMask;
    sim::step(++t);
    if (!inIntersection) {
      pidResumed = true;
      assert(previousError == 0);  // PID tabanı, teyitli ileri hatla eşitlendi
      assert(outputPWML == outputPWMR);
      break;
    }
    assert(outputPWML == outputPWMR);
  }

  printf("[T12] loop kesişimi: yanal kolda %lu ms düz geçiş, ileri hat teyidinde PID geri dönüş=%d\n",
         t, (int)pidResumed);
  assert(pidResumed);
}

//  =====================================================================
//   M A I N
//  =====================================================================
int main() {
  setbuf(stdout, NULL);
  printf("gpt-takim-v1.2: kesikli cizgi ve kesişim davranis testi\n");
  printf("Model kabadir (PWM->hiz dogrusal); amac gercek .ino kontrol mantiginin regresyonu.\n\n");

  testStraightDashCrossing();
  testDashBridgeWindow();
  testCornerLossStillPivots();
  testCornerSearchProvisionalTrack();
  testForeignLineRejectedInDashWindow();
  testLineEndDoesNotRunaway();
  testLineTrackingConverges();
  testFullBlackFloorStops();
  testBrakeLatchDuringRamp();
  testPreVacuum();
  testCurveExitEdgeContactDoesNotReverse();
  testCurveExitBlindGuidedCoast();
  testIntersectionPassesStraightBeforePid();

  printf("\nTum testler GECTI.\n");
  return 0;
}
