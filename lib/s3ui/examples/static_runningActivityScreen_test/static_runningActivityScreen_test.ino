
// RunningActivity screen test using PCF8814 and Display wrapper (static bitmap, non-animated)

#include <Arduino.h>
#include "s3ui.h"
#include "PCF8814.h"
#include "Fonts/Picopixel.h"

// Simple 24x24 bitmap to render in the activity screen
static const unsigned char PROGMEM image_ActivityBitmap_bits[] = {
	0x00,0x00,0x00,0x07,0xff,0xf0,0x04,0x00,0x10,0x03,0xff,0xe0,
	0x01,0x00,0x40,0x01,0x00,0x40,0x01,0x7f,0x40,0x01,0x3e,0x40,
	0x00,0x9c,0x80,0x00,0x49,0x00,0x00,0x22,0x00,0x00,0x14,0x00,
	0x00,0x14,0x00,0x00,0x22,0x00,0x00,0x49,0x00,0x00,0x80,0x80,
	0x01,0x08,0x40,0x01,0x3e,0x40,0x01,0x7f,0x40,0x01,0x00,0x40,
	0x03,0xff,0xe0,0x04,0x00,0x10,0x07,0xff,0xf0,0x00,0x00,0x00
};

static const uint16_t kBitmapW = 24;
static const uint16_t kBitmapH = 24;

// Pins for Nokia 1100 (PCF8814) display (SCE, SCLK, SDIN, RST)
static PCF8814 lcd(19, 18, 23, 21);
static s3ui ui;

// Test case structure for static running activity
struct StaticActivityCase {
	const char* title;
	const char* caption;
};

static StaticActivityCase kCases[] = {
	// 1) No wrap (short text)
	{"Running 1/5", "Ready."},
	// 2) Wrap (long text)
	{"Running 2/5", "Scanning channels and measuring RSSI values to optimize selection of the best link quality across nodes. This sentence should wrap nicely across multiple lines to exercise the auto-wrap logic."},
	// 3) Explicit newline (\n)
	{"Running 3/5", "Step 1: Initialize\nStep 2: Scan"},
	// 4) Carriage return (\r)
	{"Running 4/5", "Phase A\rPhase B"},
	// 5) Mixed long + newline
	{"Running 5/5", "Finding channels and scanning spectrum occupancy to avoid interference. The following description should wrap into multiple lines below the bitmap.\nScanning 1, 2, 3..."}
};

static const uint8_t kNumCases = sizeof(kCases) / sizeof(kCases[0]);
static uint8_t currentCase = 0;
static unsigned long lastStep = 0;
static const uint16_t stepMs = 1500; // 1.5s per case

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

	// Initial render: first case
	ui.runningActivityScreen(kCases[currentCase].title, "98%", image_ActivityBitmap_bits, kBitmapW, kBitmapH, kCases[currentCase].caption);
	lcd.display();
}

void loop() {
	// Cycle through cases to demonstrate no-wrap, wrap, and newline handling
	unsigned long now = millis();
	if (now - lastStep >= stepMs) {
		lastStep = now;

		currentCase++;
		if (currentCase >= kNumCases) {
			currentCase = 0;
		}

		ui.runningActivityScreen(kCases[currentCase].title, "98%", image_ActivityBitmap_bits, kBitmapW, kBitmapH, kCases[currentCase].caption);
		lcd.display();
	}
}
