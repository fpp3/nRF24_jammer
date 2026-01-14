#include "jam.h"

typedef struct {
  RF24 *radio;
  uint8_t ce_pin;
  uint8_t csn_pin;
  rf24_pa_dbm_e pa_level;
} __radio_t;

static void __init_radios();
static void __deinit_radios(bool stopConstCarrier);
static void __hspi_init();
static void __hspi_deinit();

static SPIClass *__hp = nullptr;
static uint8_t __nrf24_count; // Define the number of NRF24 radios
static __radio_t *__radios = nullptr;
static bool __constCarrier = false;
static jam_mode_t __jam = JAMMING_IDLE;
static bool __is_initialized = false;
static jam_tx_mode_t __mode = JAM_TX_STANDALONE;

static const uint8_t bluetooth_channels[] PROGMEM = {32, 34, 46, 48, 50, 52, 0,  1,  2,  4, 6,
                                                     8,  22, 24, 26, 28, 30, 74, 76, 78, 80};
static const uint8_t ble_channels[] PROGMEM = {2, 26, 80};
static const char jam_text[] PROGMEM = "xxxxxxxxxxxxxxxx";

// Local functions. Not to be used by any other code outside this file.

void __init_radios() {
  for (int radio_i = 0; radio_i < __nrf24_count; radio_i++) {
    __radios[radio_i].radio = new RF24(__radios[radio_i].ce_pin, __radios[radio_i].csn_pin);
  }
}

void __deinit_radios(bool stopConstCarrier) {
  for (int radio_i = 0; radio_i < __nrf24_count; radio_i++) {
    if (__radios[radio_i].radio != nullptr) {
      if (stopConstCarrier) {
        __radios[radio_i].radio->stopConstCarrier();
      }
      __radios[radio_i].radio->powerDown();
      delete __radios[radio_i].radio;
      __radios[radio_i].radio = nullptr;
    }
  }
}

void __hspi_init() {
  __hp = new SPIClass(HSPI);
  __hp->begin();
  __hp->setFrequency(16000000);
  __hp->setBitOrder(MSBFIRST);
  __hp->setDataMode(SPI_MODE0);
  for (int radio_i = 0; radio_i < __nrf24_count; radio_i++) {
    __radios[radio_i].radio->begin(__hp);
    __radios[radio_i].radio->setAutoAck(false);
    __radios[radio_i].radio->stopListening();
    __radios[radio_i].radio->setRetries(0, 0);
    __radios[radio_i].radio->setPayloadSize(5);
    __radios[radio_i].radio->setAddressWidth(3);
    __radios[radio_i].radio->setPALevel(__radios[radio_i].pa_level, true);
    __radios[radio_i].radio->setDataRate(RF24_2MBPS);
    __radios[radio_i].radio->setCRCLength(RF24_CRC_DISABLED);
  }
}

void __hspi_deinit() {
  if (__hp != nullptr) {
    __hp->end();
    delete __hp;
    __hp = nullptr;
  }
}

// Global functions. To be used by anyone including jam.h

bool load_radios(nrf24_pins_t *pins_array, uint8_t count) {
  if (__radios != nullptr) {
    return false; // Already loaded
  }
  __nrf24_count = count;
  __radios = new __radio_t[__nrf24_count];
  for (int radio_i = 0; radio_i < __nrf24_count; radio_i++) {
    __radios[radio_i].ce_pin = pins_array[radio_i].ce_pin;
    __radios[radio_i].csn_pin = pins_array[radio_i].csn_pin;
    if (pins_array[radio_i].pa_level > RF24_PA_MAX) {
      return false; // Invalid PA level
    }
    __radios[radio_i].pa_level = pins_array[radio_i].pa_level;
    __radios[radio_i].radio = nullptr;
  }
  return true;
}

void set_jam_tx_mode(jam_tx_mode_t mode) { __mode = mode; }

void jam_start() {
  if (!__is_initialized) {
    __init_radios();
    __hspi_init();
  }
  if (__constCarrier) {
    for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
      __radios[radio_i].radio->startConstCarrier(RF24_PA_MAX, 45);
    }
  }
  __is_initialized = true;
}

void jam_stop() {
  if (__is_initialized) {
    __deinit_radios(__constCarrier);
    __hspi_deinit();
    __constCarrier = false;
    __is_initialized = false;
    __jam = JAMMING_IDLE;
  }
}

void bluetooth_jam(uint8_t method) {
  if (__jam == JAMMING_IDLE) {
    jam_start();
    __constCarrier = true;
    __jam = JAMMING_BLUETOOTH;
  }
  if (__jam == JAMMING_BLUETOOTH) {
    if (__mode == JAM_TX_STANDALONE) {
      uint8_t total_channels = method == 0 ? 21 : 80;
      uint8_t base = total_channels / __nrf24_count;
      uint8_t rem = total_channels % __nrf24_count;
      uint8_t ch = 0;
      for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
        uint8_t count = base + (radio_i < rem ? 1 : 0);
        for (uint8_t chPerRadio = 0; chPerRadio < count; chPerRadio++, ch++) {
          switch (method) {
            case 0:
              __radios[radio_i].radio->setChannel(bluetooth_channels[ch]);
              break;
            case 1:
              __radios[radio_i].radio->setChannel(random(total_channels));
              break;
            case 2:
              __radios[radio_i].radio->setChannel(ch);
              break;
          }
        }
      }
    } else {
      uint8_t total_channels = method == 0 ? 21 : 80;
      for (uint8_t ch = 0; ch < total_channels; ch++) {
        uint8_t channel;
        switch (method) {
          case 0:
            channel = bluetooth_channels[ch];
            break;
          case 1:
            channel = random(total_channels);
            break;
          case 2:
            channel = ch;
            break;
        }
        for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
          __radios[radio_i].radio->setChannel(channel);
        }
      }
    }
  }
}

void drone_jam(uint8_t method) {
  if (__jam == JAMMING_IDLE) {
    jam_start();
    __constCarrier = true;
    __jam = JAMMING_DRONE;
  }
  if (__jam == JAMMING_DRONE) {
    if (__mode == JAM_TX_STANDALONE) {
      uint8_t total_channels = 125;
      uint8_t base = total_channels / __nrf24_count;
      uint8_t rem = total_channels % __nrf24_count;
      uint8_t ch = 0;
      for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
        uint8_t count = base + (radio_i < rem ? 1 : 0);
        for (uint8_t chPerRadio = 0; chPerRadio < count; chPerRadio++, ch++) {
          switch (method) {
            case 0:
              __radios[radio_i].radio->setChannel(random(total_channels));
              break;
            case 1:
              __radios[radio_i].radio->setChannel(ch);
              break;
          }
        }
      }
    } else {
      uint8_t total_channels = 125;
      for (uint8_t ch = 0; ch < total_channels; ch++) {
        uint8_t channel;
        switch (method) {
          case 0:
            channel = random(total_channels);
            break;
          case 1:
            channel = ch;
            break;
        }
        for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
          __radios[radio_i].radio->setChannel(channel);
        }
      }
    }
  }
}

void ble_jam() {
  if (__jam == JAMMING_IDLE) {
    jam_start();
    __constCarrier = false;
    __jam = JAMMING_BLE;
  }
  if (__jam == JAMMING_BLE) {
    if (__mode == JAM_TX_STANDALONE) {
      uint8_t total_channels = 3;
      uint8_t base = total_channels / __nrf24_count;
      uint8_t rem = total_channels % __nrf24_count;
      uint8_t ch = 0;
      for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
        uint8_t count = base + (radio_i < rem ? 1 : 0);
        for (uint8_t chPerRadio = 0; chPerRadio < count; chPerRadio++, ch++) {
          __radios[radio_i].radio->setChannel(ble_channels[ch]);
        }
      }
    } else {
      uint8_t total_channels = 3;
      for (uint8_t ch = 0; ch < total_channels; ch++) {
        for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
          __radios[radio_i].radio->setChannel(ble_channels[ch]);
        }
      }
    }
  }
}

void wifi_jam(int8_t channel = -1) {
  if (__jam == JAMMING_IDLE) {
    jam_start();
    __constCarrier = false;
    __jam = JAMMING_WIFI;
  }
  if (__jam == JAMMING_WIFI) {
    if (channel < 0) {
      if (__mode == JAM_TX_STANDALONE) {
        uint8_t total_channels = 22;
        uint8_t base = total_channels / __nrf24_count;
        uint8_t rem = total_channels % __nrf24_count;
        uint8_t ch = 1;
        for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
          uint8_t count = base + (radio_i < rem ? 1 : 0);
          for (uint8_t chPerRadio = 0; chPerRadio < count; chPerRadio++, ch++) {
            __radios[radio_i].radio->setChannel(ch);
            __radios[radio_i].radio->writeFast(&jam_text, sizeof(jam_text));
          }
        }
      } else {
        for (uint8_t ch = 1; ch <= 22; ch++) {
          for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
            __radios[radio_i].radio->setChannel(ch);
            __radios[radio_i].radio->writeFast(&jam_text, sizeof(jam_text));
          }
        }
      }
    } else {
      if (__mode == JAM_TX_STANDALONE) {
        uint8_t total_channels = 22;
        uint8_t base = total_channels / __nrf24_count;
        uint8_t rem = total_channels % __nrf24_count;
        uint8_t ch = (channel * 5) + 1;
        for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
          uint8_t count = base + (radio_i < rem ? 1 : 0);
          for (uint8_t i = 0; i < count; i++, ch++) {
            __radios[radio_i].radio->setChannel(ch);
            __radios[radio_i].radio->writeFast(&jam_text, sizeof(jam_text));
          }
        }
      } else {
        for (uint8_t ch = (channel * 5) + 1; ch < (channel * 5) + 23; ch++) {
          for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
            __radios[radio_i].radio->setChannel(ch);
            __radios[radio_i].radio->writeFast(&jam_text, sizeof(jam_text));
          }
        }
      }
    }
  }
}

void zigbee_jam() {
  if (__jam == JAMMING_IDLE) {
    jam_start();
    __constCarrier = false;
    __jam = JAMMING_ZIGBEE;
  }
  if (__jam == JAMMING_ZIGBEE) {
    if (__mode == JAM_TX_STANDALONE) {
      uint8_t total_channels = 16;
      uint8_t base = total_channels / __nrf24_count;
      uint8_t rem = total_channels % __nrf24_count;
      uint8_t ch = 0;
      for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
        uint8_t count = base + (radio_i < rem ? 1 : 0);
        for (uint8_t chPerRadio = 0; chPerRadio < count; chPerRadio++, ch++) {
          __radios[radio_i].radio->setChannel(4 + 5 * ch);
          __radios[radio_i].radio->writeFast(&jam_text, sizeof(jam_text));
        }
      }
    } else {
      for (uint8_t channel = 11; channel < 27; channel++) {
        for (uint8_t ch = 4 + 5 * (channel - 11); ch <= (5 + 5 * (channel - 11)) + 2; ch++) {
          for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
            __radios[radio_i].radio->setChannel(ch);
            __radios[radio_i].radio->writeFast(&jam_text, sizeof(jam_text));
          }
        }
      }
    }
  }
}

void misc_jam(uint8_t channel1, uint8_t channel2) {
  if (__jam == JAMMING_IDLE) {
    jam_start();
    __constCarrier = true;
    __jam = JAMMING_MISC;
  }
  if (__jam == JAMMING_MISC) {
    if (__mode == JAM_TX_STANDALONE) {
      uint8_t total_channels = channel2 - channel1 + 1;
      uint8_t base = total_channels / __nrf24_count;
      uint8_t rem = total_channels % __nrf24_count;
      uint8_t ch = channel1;
      for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
        uint8_t count = base + (radio_i < rem ? 1 : 0);
        for (uint8_t i = 0; i < count; i++, ch++) {
          __radios[radio_i].radio->setChannel(ch);
          __radios[radio_i].radio->writeFast(&jam_text, sizeof(jam_text));
        }
      }
    } else {
      for (uint8_t ch = channel1; ch <= channel2; ch++) {
        for (uint8_t radio_i = 0; radio_i < __nrf24_count; radio_i++) {
          __radios[radio_i].radio->setChannel(ch);
          __radios[radio_i].radio->writeFast(&jam_text, sizeof(jam_text));
        }
      }
    }
  }
}
