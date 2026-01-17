// Confirm screen test - cycles through all variations
// Demonstrates: with/without bitmap, short/long text, 1-3 options, cycling selection

#include <Arduino.h>
#include <s3ui.h>
#include <PCF8814.h>
#include <Fonts/Picopixel.h>

// Pins for Nokia 1100 (PCF8814) display (SCE, SCLK, SDIN, RST)
static PCF8814 lcd(19, 18, 23, 21);
static s3ui ui;

// Simple 16x16 checkerboard bitmap for testing
static const unsigned char PROGMEM checkerboard_16x16[] = {
  0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55,
  0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55,
  0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55,
  0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55
};

// 20x20 checkerboard for variety
static const unsigned char PROGMEM checkerboard_20x20[] = {
  0xAA, 0xA0, 0x55, 0x50, 0xAA, 0xA0, 0x55, 0x50, 0xAA, 0xA0,
  0x55, 0x50, 0xAA, 0xA0, 0x55, 0x50, 0xAA, 0xA0, 0x55, 0x50,
  0xAA, 0xA0, 0x55, 0x50, 0xAA, 0xA0, 0x55, 0x50, 0xAA, 0xA0,
  0x55, 0x50, 0xAA, 0xA0, 0x55, 0x50, 0xAA, 0xA0, 0x55, 0x50,
  0xAA, 0xA0, 0x55, 0x50, 0xAA, 0xA0, 0x55, 0x50, 0xAA, 0xA0
};

// Test case structure
struct TestCase {
  const char* title;
  const char* question;
  String options[3];
  uint8_t numOptions;
  const uint8_t* bitmap;
  uint16_t bitmapW;
  uint16_t bitmapH;
  bool hasBitmap;
};

// All test variations
static TestCase testCases[] = {
  // 1. No bitmap, short text, 1 option
  {
    "Test 1/18", 
    "Continue?", 
    {"OK"}, 
    1, 
    nullptr, 0, 0, 
    false
  },
  
  // 2. No bitmap, short text, 2 options
  {
    "Test 2/18", 
    "Are you sure?", 
    {"Yes", "Cancel"}, 
    2, 
    nullptr, 0, 0, 
    false
  },
  
  // 3. No bitmap, short text, 3 options
  {
    "Test 3/18", 
    "Choose action:", 
    {"Save", "Discard", "Cancel"}, 
    3, 
    nullptr, 0, 0, 
    false
  },
  
  // 4. No bitmap, long text (will wrap), 2 options
  {
    "Test 4/18", 
    "This text is intentionally very long and will be wrapped to multiple lines", 
    {"Proceed", "Abort"}, 
    2, 
    nullptr, 0, 0, 
    false
  },
  
  // 5. With bitmap (16x16), short text, 1 option
  {
    "Test 5/18", 
    "Processing...", 
    {"Wait"}, 
    1, 
    checkerboard_16x16, 16, 16, 
    true
  },
  
  // 6. With bitmap (16x16), short text, 2 options
  {
    "Test 6/18", 
    "Reset to defaults?", 
    {"Reset", "Cancel"}, 
    2, 
    checkerboard_16x16, 16, 16, 
    true
  },
  
  // 7. With bitmap (20x20), short text, 3 options
  {
    "Test 7/18", 
    "Save settings?", 
    {"Save", "Discard", "Review"}, 
    3, 
    checkerboard_20x20, 20, 20, 
    true
  },
  
  // 8. With bitmap (20x20), long text, 3 options
  {
    "Test 8/18", 
    "This action cannot be undone and will delete all your data permanently", 
    {"Delete", "Keep", "Backup"}, 
    3, 
    checkerboard_20x20, 20, 20, 
    true
  },
  
  // 9. With bitmap (16x16), long text, 2 options
  {
    "Test 9/18", 
    "A new version is available with many improvements and bug fixes", 
    {"Update", "Later"}, 
    2, 
    checkerboard_16x16, 16, 16, 
    true
  },
  
  // 10. No bitmap, short text, 3 options (different labels)
  {
    "Test 10/18", 
    "Exit application?", 
    {"Yes", "No", "Minimize"}, 
    3, 
    nullptr, 0, 0, 
    false
  },
  
  // === Smart Layout Tests ===
  
  // 11. Short buttons - all fit horizontally (1 row)
  {
    "Test 11/18", 
    "All fit side by side:", 
    {"A", "B", "C"}, 
    3, 
    nullptr, 0, 0, 
    false
  },
  
  // 12. Medium buttons - 2 on top, 1 on bottom
  {
    "Test 12/18", 
    "Two top, one bottom:", 
    {"Accept", "Decline", "Should i really?"}, 
    3, 
    nullptr, 0, 0, 
    false
  },
  
  // 13. Long buttons - must stack vertically
  {
    "Test 13/18", 
    "Stacked vertically:", 
    {"Configuration", "Initialization", "Termination"}, 
    3, 
    nullptr, 0, 0, 
    false
  },
  
  // 14. Two short buttons - horizontal layout
  {
    "Test 14/18", 
    "Two short buttons:", 
    {"OK", "Cancel"}, 
    2, 
    nullptr, 0, 0, 
    false
  },
  
  // 15. Two long buttons - vertical stack
  {
    "Test 15/18", 
    "Two long buttons:", 
    {"Continue Process", "Abort Operation"}, 
    2, 
    nullptr, 0, 0, 
    false
  },
  
  // 16. With bitmap + smart layout (short buttons)
  {
    "Test 16/18", 
    "With bitmap, all fit:", 
    {"Go", "Stop", "Wait"}, 
    3, 
    checkerboard_16x16, 16, 16, 
    true
  },
  
  // 17. With bitmap + 2+1 layout
  {
    "Test 17/18", 
    "Bitmap, 2 top 1 bottom:", 
    {"Apply", "Cancel", "Consider it"}, 
    3, 
    checkerboard_20x20, 20, 20, 
    true
  },
  
  // 18. With bitmap + vertical stack (long labels)
  {
    "Test 18/18", 
    "Bitmap, vertical:", 
    {"Activate Device", "Deactivate Now", "Configuration"}, 
    3, 
    checkerboard_16x16, 16, 16, 
    true
  }
};

static const uint8_t numTestCases = sizeof(testCases) / sizeof(testCases[0]);

// State machine
static uint8_t currentTest = 0;
static uint8_t selectedOption = 0;
static unsigned long lastStep = 0;
static const uint16_t stepMs = 800; // 0.8s per step

void renderCurrentTest() {
  TestCase& test = testCases[currentTest];
  
  if (test.hasBitmap && test.bitmap) {
    ui.confirmScreen(
      test.title, 
      "87%", 
      test.bitmap, 
      test.bitmapW, 
      test.bitmapH, 
      test.question, 
      test.options, 
      test.numOptions, 
      selectedOption
    );
  } else {
    ui.confirmScreen(
      test.title, 
      "87%", 
      test.question, 
      test.options, 
      test.numOptions, 
      selectedOption
    );
  }
  
  lcd.display();
}

void setup() {
  // Initialize display
  lcd.begin();
  lcd.setContrast(0x0F); // Adjust if needed for your module
  lcd.displayOn();

  // Initialize UI and fonts
  ui.setDisplay(&lcd, 96, 65);
  ui.setTitleFont(&Picopixel);
  ui.setContentFont(&Picopixel);
  ui.setTitleSize(1);
  ui.setContentSize(1);
  // Initial render
  renderCurrentTest();
}

void loop() {
  unsigned long now = millis();
  
  if (now - lastStep >= stepMs) {
    lastStep = now;
    
    // Cycle through selected options for current test
    selectedOption++;
    if (selectedOption >= testCases[currentTest].numOptions) {
      selectedOption = 0;
      
      // Move to next test case
      currentTest++;
      if (currentTest >= numTestCases) {
        currentTest = 0;
      }
    }
    
    renderCurrentTest();
  }
  
  // Allow UI to update (not needed for confirm screens, but good practice)
  ui.update();
  // Small delay to prevent a tight loop from consuming unnecessary CPU/power
  delay(10);
}
