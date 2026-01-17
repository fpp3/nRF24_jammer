// Simple OptionSelect screen test using PCF8814 and s3ui wrapper

#include <s3ui.h>
#include <PCF8814.h>
#include <Picopixel.h>

// Pins for Nokia 1100 (PCF8814) display (SCE, SCLK, SDIN, RST)
static PCF8814 lcd(19, 18, 23, 21);
static s3ui ui;

// Demo data
static String title = "Option Select";
static String options[] = {
    "Scan",
    "Jammer",
    "Channels",
    "Settings",
    "About",
    "Option1",
    "Option2",
    "Option3",
    "Option4",
    "Option5",
    "Option6",  
    "Option7",
    "Option8",
    "Option9",
    "Option10"
};
static const uint8_t numOptions = sizeof(options) / sizeof(options[0]);

// Cursor animation state
static uint8_t cursor = 0;
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
  ui.optionSelectScreen(title, "99%", options, numOptions, cursor);
  lcd.display();
}

void loop() {
  // Auto-advance cursor to demonstrate non-blocking UI
  unsigned long now = millis();
  if (now - lastStep >= stepMs) {
    lastStep = now;
    cursor = (uint8_t)((cursor + 1) % numOptions);
    ui.optionSelectScreen(title, "99%", options, numOptions, cursor);
    lcd.display();
  }

  // Allow s3ui to update any animations (none on OptionSelect)
  ui.update();
  
  // Small delay to prevent overwhelming the MCU
  delay(10);
}
