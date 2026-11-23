#include <Arduino.h>
#include <nrf.h>

namespace {
constexpr unsigned kDataPin = 9;  // P0.09 on the MCU
constexpr unsigned kPixels = 231;  // LED1 through LED231
constexpr uint8_t kBlue = 8;
constexpr uint16_t kLow = 0x8000;
constexpr uint16_t kZero = kLow | 5;
constexpr uint16_t kOne = kLow | 10;
constexpr unsigned kLatchPeriods = 321;
constexpr unsigned kFrameWords = kPixels * 24 + kLatchPeriods;
constexpr uint32_t kDisconnected = PWM_PSEL_OUT_CONNECT_Msk;

alignas(4) uint16_t frame[kFrameWords];
const char *status = "STARTING";
bool pwmFault = false;

void configurePwm() {
  NRF_P0->OUTCLR = 1UL << kDataPin;
  NRF_P0->DIRSET = 1UL << kDataPin;
  NRF_PWM0->ENABLE = 0;
  for (auto &output : NRF_PWM0->PSEL.OUT) {
    output = kDisconnected;
  }
  NRF_PWM0->MODE = PWM_MODE_UPDOWN_Up;
  NRF_PWM0->PRESCALER = PWM_PRESCALER_PRESCALER_DIV_1;
  NRF_PWM0->COUNTERTOP = 20;  // 1.25 us per bit
  NRF_PWM0->DECODER = PWM_DECODER_LOAD_Common;
  NRF_PWM0->LOOP = 0;
  NRF_PWM0->SHORTS = 0;
  NRF_PWM0->SEQ[0].PTR = reinterpret_cast<uint32_t>(frame);
  NRF_PWM0->SEQ[0].CNT = kFrameWords;
  NRF_PWM0->SEQ[0].REFRESH = 0;
  NRF_PWM0->SEQ[0].ENDDELAY = 0;
}

void fillFrame(uint8_t blue) {
  const uint8_t grb[] = {0, 0, blue};
  auto *word = frame;
  for (unsigned pixel = 0; pixel < kPixels; ++pixel) {
    for (const uint8_t channel : grb) {
      for (unsigned mask = 0x80; mask != 0; mask >>= 1) {
        *word++ = channel & mask ? kOne : kZero;
      }
    }
  }
  while (word != frame + kFrameWords) {
    *word++ = kLow;
  }
}

bool waitFor(volatile uint32_t &event, uint32_t timeoutMs) {
  const uint32_t start = millis();
  while (!event) {
    if (millis() - start >= timeoutMs) {
      return false;
    }
    delay(1);
  }
  return true;
}

bool show() {
  NRF_PWM0->EVENTS_SEQEND[0] = 0;
  NRF_PWM0->EVENTS_STOPPED = 0;
  NRF_PWM0->PSEL.OUT[0] = kDataPin;
  NRF_PWM0->ENABLE = PWM_ENABLE_ENABLE_Enabled;
  __DMB();
  NRF_PWM0->TASKS_SEQSTART[0] = 1;
  // At SEQEND, the final low sample starts after 320 complete low periods.
  const bool completed = waitFor(NRF_PWM0->EVENTS_SEQEND[0], 30);
  NRF_PWM0->TASKS_STOP = 1;
  const bool stopped = waitFor(NRF_PWM0->EVENTS_STOPPED, 10);
  if (stopped) {
    NRF_PWM0->ENABLE = 0;
  }
  NRF_PWM0->PSEL.OUT[0] = kDisconnected;
  if (!stopped || !completed) {
    pwmFault = true;
    status = stopped ? "FRAME_TIMEOUT" : "STOP_TIMEOUT";
    return false;
  }
  status = "PWM_DONE";
  return true;
}
}  // namespace

void setup() {
  Serial.begin(115200);
  if ((NRF_UICR->NFCPINS & UICR_NFCPINS_PROTECT_Msk) != 0) {
    pwmFault = true;
    status = "NFC_PINS_NOT_GPIO";
    return;
  }
  configurePwm();
  delay(100);
  fillFrame(0);
  if (show()) {
    fillFrame(kBlue);
    show();
  }
}

void loop() {
  while (Serial.available()) {
    if (Serial.read() == '?') {
      Serial.print(status);
      Serial.print(" PIXELS=");
      Serial.print(kPixels);
      Serial.print(" P0.09 GRB B=");
      Serial.println(kBlue);
    }
  }
  if (!pwmFault) {
    show();  // Restore blue if the independently powered LED board restarts.
  }
  delay(250);
}
