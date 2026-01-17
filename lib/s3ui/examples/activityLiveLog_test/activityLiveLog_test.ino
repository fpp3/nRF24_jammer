// ActivityLiveLog screen test using PCF8814 and s3ui wrapper
// Tests log scrolling, text wrapping, and dynamic log appending

#include <s3ui.h>
#include <PCF8814.h>
#include <Picopixel.h>

// Pins for Nokia 1100 (PCF8814) display (SCE, SCLK, SDIN, RST)
static PCF8814 lcd(19, 18, 23, 21);
static s3ui ui;

// Test log messages - some are long to test text wrapping
static String testMessages[] = {
    "System initialized",
    "Scanning for devices...",
    "Device found: RF24-001 at channel 10",
    "Long message that should wrap to multiple lines because it contains a lot of text to demonstrate the text wrapping feature",
    "Connecting to Device RF24-001...",
    "Connection established. Signal strength: -45 dBm",
    "This is another very long message that will test the automatic text wrapping capability of the log display system",
    "Starting data transmission",
    "Transmitted 1024 bytes successfully",
    "Error: Timeout on channel 12",
    "Retrying connection with exponential backoff...",
    "Successfully reconnected to Device RF24-001 after 3 attempts",
    "Message with\nnewline\ncharacters to test line breaks",
    "Battery level: 87%",
    "Device RF24-001 disconnected unexpectedly",
    "Attempting auto-reconnect...",
    "Successfully reconnected after 2 seconds",
    "All systems operational. Waiting for commands from user interface"
};

static const uint8_t kNumMessages = sizeof(testMessages) / sizeof(testMessages[0]);

// Timing and state for log generation
static unsigned long lastLogTime = 0;
static const uint16_t logIntervalMs = 1500;  // Add a new log every 1.5 seconds
static uint8_t nextMessageIndex = 0;
static unsigned long cycleSwitchTime = 0;
static const uint16_t cycleDurationMs = 30000;  // Clear and restart logs every 30 seconds

void setup() {
  // Initialize display
  Serial.begin(115200);
  lcd.begin();
  lcd.setContrast(0x0F);  // Adjust if needed for your module
  lcd.displayOn();

  // Initialize wrapper and fonts
  ui.setDisplay(&lcd, 96, 65);
  ui.setTitleFont(&Picopixel);
  ui.setContentFont(&Picopixel);
  ui.setTitleSize(1);
  ui.setContentSize(1);

  // Initialize the activity log screen
  ui.activityLiveLogScreen("Activity Log", "95%");
  
  // Add initial messages
  ui.appendLogLine("System starting...");
  ui.appendLogLine("Initializing hardware");
  
  lastLogTime = millis();
  cycleSwitchTime = millis();
  nextMessageIndex = 0;
  
  lcd.display();
}

void loop() {
  unsigned long now = millis();
  // Check if we should clear and restart the log cycle
  if (now - cycleSwitchTime >= cycleDurationMs) {
    ui.clearLog();
    ui.appendLogLine("=== Log Cycle Restarted ===");
    cycleSwitchTime = now;
    lastLogTime = now;
    nextMessageIndex = 0;
  }
  
  // Add a new log message at regular intervals
  if (now - lastLogTime >= logIntervalMs) {
    lastLogTime = now;
    
    // Append the next message
    if (nextMessageIndex < kNumMessages) {
      ui.appendLogLine(testMessages[nextMessageIndex]);
      nextMessageIndex++;
    } else {
      // Cycle through messages again
      nextMessageIndex = 0;
      ui.appendLogLine("--- Repeating messages ---");
    }
  }
  
  // Update the display (handles log screen refresh)
  ui.update();
  lcd.display();
  
  // Small delay to prevent overwhelming the MCU
  delay(10);
}
