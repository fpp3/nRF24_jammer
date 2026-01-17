// OptionValueSet screen test using PCF8814 and s3ui wrapper

#include <s3ui.h>
#include <PCF8814.h>
#include <Picopixel.h>

// Pins for Nokia 1100 (PCF8814) display (SCE, SCLK, SDIN, RST)
static PCF8814 lcd(19, 18, 23, 21);
static s3ui ui;

// Demo data for OptionValueSet
static String title = "Settings";
static String optionNames[] = {
  "Brightness",
  "Volume",
  "Channel",
  "Power Mode",
  "Timeout",
  "Contrast",
  "Language"
};
static const uint8_t numOptions = sizeof(optionNames) / sizeof(optionNames[0]);

// Value options for each setting
static String brightnessValues[] = {"25%", "50%", "75%", "100%"};
static String volumeValues[] = {"0%", "40%", "80%", "100%"};
static String channelValues[] = {"1", "5", "10", "15", "20"};
static String powerModeValues[] = {"Low", "Medium", "High", "Ultra"};
static String timeoutValues[] = {"30s", "60s", "120s", "300s"};
static String contrastValues[] = {"5", "10", "15", "20", "25"};
static String languageValues[] = {"English", "Español", "Français", "Deutsch"};

// Current value indices for each option
static uint8_t valueIndices[] = {2, 2, 2, 1, 1, 2, 0};  // Default indices

// Value arrays for each option
static String *allValues[] = {
  brightnessValues,
  volumeValues,
  channelValues,
  powerModeValues,
  timeoutValues,
  contrastValues,
  languageValues
};

// Number of values for each option
static uint8_t valueCount[] = {
  sizeof(brightnessValues) / sizeof(brightnessValues[0]),
  sizeof(volumeValues) / sizeof(volumeValues[0]),
  sizeof(channelValues) / sizeof(channelValues[0]),
  sizeof(powerModeValues) / sizeof(powerModeValues[0]),
  sizeof(timeoutValues) / sizeof(timeoutValues[0]),
  sizeof(contrastValues) / sizeof(contrastValues[0]),
  sizeof(languageValues) / sizeof(languageValues[0])
};

// Build the current optionValues array
static String optionValues[numOptions];

void updateOptionValues() {
  for (uint8_t i = 0; i < numOptions; i++) {
    optionValues[i] = allValues[i][valueIndices[i]];
  }
}

// Cursor animation state
static uint8_t position = 0;
static bool optionSelected = false;
static unsigned long lastStep = 0;
static const uint16_t stepMs = 700; // 0.7s per step

void setup() {
  // Initialize display
  lcd.begin();
  lcd.setContrast(0x0F); // Adjust if needed for your module
  lcd.displayOn();

  // Initialize wrapper and fonts
  ui.setDisplay(&lcd, 96, 65);
  ui.setTitleFont(&Picopixel);
  ui.setContentFont(&Picopixel);
  ui.setTitleSize(1);
  ui.setContentSize(1);

  // Initial render
  updateOptionValues();
  ui.optionValueSetScreen(title, "99%", optionNames, optionValues, numOptions, position, optionSelected);
  lcd.display();
}

void loop() {
  // Navigate through options and cycle through values when selected
  unsigned long now = millis();
  if (now - lastStep >= stepMs) {
    lastStep = now;
    
    if (optionSelected) {
      // Cycle to next value for current option
      valueIndices[position] = (uint8_t)((valueIndices[position] + 1) % valueCount[position]);
      updateOptionValues();
      
      // If we've cycled through all values (wrapped around to 0), move to next option
      if (valueIndices[position] == 0) {
        position = (uint8_t)((position + 1) % numOptions);
        optionSelected = true;
      }
    } else {
      // Move to next option
      position = (uint8_t)((position + 1) % numOptions);
      optionSelected = true;
    }
    
    ui.optionValueSetScreen(title, "99%", optionNames, optionValues, numOptions, position, optionSelected);
    lcd.display();
  }

  // Allow s3ui to update any animations
  ui.update();
  
  // Small delay to prevent overwhelming the MCU
  delay(10);
}
