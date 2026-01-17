#include "Arduino.h"
#include "Preferences.h"

#include "Button2.h"
#include "Fonts/Picopixel.h"
#include "PCF8814.h"
#include "s3ui.h"

#include "bitmaps.h"
#include "jam.h"

#define PIN_VBAT 36
#define PIN_OK 25
#define PIN_NEXT 26
#define PIN_PREV 27
#define VBAT_VOLTAGE_EMPTY 2.8f
#define VBAT_VOLTAGE_FULL 3.3f

s3ui ui;
PCF8814 display(19, 18, 23, 21);
Button2 buttonOk;
Button2 buttonNext;
Button2 buttonPrev;

const uint8_t displayWidth = 96;
const uint8_t displayHeight = 65;
const char *CONFIG_NAMESPACE PROGMEM = "jammer";
const char *CONFIG_KEY_RADIO_NUM PROGMEM = "radio_count";
const char *CONFIG_KEY_RADIO_CONFIG_STRUCT PROGMEM = "radio_cfg";
const char *CONFIG_KEY_RADIO_JAM_MODE PROGMEM = "radio_jam_mode";
const char *CONFIG_MANDATORY_KEYS[] PROGMEM = {CONFIG_KEY_RADIO_NUM, CONFIG_KEY_RADIO_JAM_MODE, nullptr};

const String option_allJam PROGMEM = "All channels (sequential)";
const String option_randomJam PROGMEM = "All channels (random)";
const String option_return PROGMEM = "Return";

const String title_menu_main PROGMEM = "Main Menu";
const String title_menu_btJam PROGMEM = "Bluetooth Jam";
const String title_menu_wifiJam PROGMEM = "WiFi Jam";
const String title_menu_bleJam PROGMEM = "BLE Jam";
const String title_menu_zigbeeJam PROGMEM = "Zigbee Jam";
const String title_menu_droneJam PROGMEM = "Drone Jam";
const String title_menu_miscJam PROGMEM = "Misc Jam";
const String title_menu_settings PROGMEM = "Settings";
const String title_menu_about PROGMEM = "About";
const String title_menu_radios PROGMEM = "Radios";
const String title_menu_jammingModes PROGMEM = "Jamming Mode";

const String menu_main[] PROGMEM = {title_menu_btJam,    title_menu_wifiJam, title_menu_bleJam,   title_menu_zigbeeJam,
                                    title_menu_droneJam, title_menu_miscJam, title_menu_settings, title_menu_about};
const String menu_btJam[] PROGMEM = {"Channel List (21)", option_allJam, option_randomJam, option_return};
const String menu_wifiJam[] PROGMEM = {option_allJam, "Single channel", option_return};
const String menu_droneJam[] PROGMEM = {option_randomJam, option_allJam, option_return};
const String menu_settings[] PROGMEM = {title_menu_radios, title_menu_jammingModes, "Factory Reset", option_return};
const String menu_jammingModes[] PROGMEM = {"Simultaneous", "Standalone"};

/*
 ============================================================================
                      MENU SYSTEM ARCHITECTURE
 ============================================================================

 This is a hierarchical, non-blocking menu system with up to 3 depth levels.
 Each menu item can either:
   a) Navigate to a submenu (has nullptr check)
   b) Execute a leaf action (returns bool for completion status)

 STRUCTURE:
   - Menu items are stored as String arrays (e.g., menu_main, menu_btJam)
   - Menu sizes are pre-calculated in size arrays to avoid repetitive sizeof()
   - Menu tree pointers form a 3D navigation structure indexed by [depth][position]
   - Action functions are stored in parallel arrays matching the menu structure

 NAVIGATION:
   - menuDepth: Current depth (0=main, 1=submenu, 2=sub-submenu)
   - menuPositions[3]: Current selection at each depth level
   - Next/Prev buttons: Navigate within current menu (wrapping)
   - OK button: Either enter submenu or execute leaf action
   - Return option: Go back to previous depth (automatic)

 ACTIONS (Non-Blocking):
   - Action functions return bool:
     * false = still running (display updated, stay in menu)
     * true = complete (go back to previous depth)
   - Each action manages its own state using static variables
   - Use millis() timer instead of delay() to keep loop responsive
   - Buttons remain responsive during action execution

 ADDING NEW MENUS/ACTIONS:
   1. Create menu array (const String menu_name[])
   2. Add size to depth_*_menuSizes arrays
   3. Add pointer to depth_*_menuTree arrays
   4. Create action function (bool action_name())
   5. Add function pointer to depth_*_actions arrays
   6. Add forward declaration

 ============================================================================
 */
const String *depth_0_menuTree PROGMEM = menu_main;
const String depth_0_titleTree[] PROGMEM = {title_menu_main};
const String *depth_1_menuTree[] PROGMEM = {
    menu_btJam,    // Submenu for "Bluetooth Jam"
    menu_wifiJam,  // Submenu for "WiFi Jam"
    nullptr,       // No submenu for "BLE Jam"
    nullptr,       // No submenu for "Zigbee Jam"
    menu_droneJam, // Submenu for "Drone Jam"
    nullptr,       // No submenu for "Misc Jam"
    menu_settings, // Submenu for "Settings"
    nullptr        // No submenu for "About"
};
const String *depth_1_titleTree PROGMEM = menu_main; // Order is the same, so no need to redefine it.

// Depth 2 sub-arrays (one per depth_1 menu that has children)
const String *depth_2_btJam[] PROGMEM = {
    nullptr,  // "Channel List (21)"
    nullptr,  // "All channels (sequential)"
    nullptr,  // "All channels (random)"
    nullptr   // "Return"
};

const String *depth_2_wifiJam[] PROGMEM = {
    nullptr,  // "All channels (random)"
    nullptr,  // "Single channel"
    nullptr   // "Return"
};

const String *depth_2_droneJam[] PROGMEM = {
    nullptr,  // "All channels (random)"
    nullptr,  // "All channels (sequential)"
    nullptr   // "Return"
};

const String *depth_2_settings[] PROGMEM = {
    nullptr,           // "Radios"
    menu_jammingModes, // "Jamming Mode" - has submenu!
    nullptr,           // "Factory Reset"
    nullptr            // "Return"
};

// Depth 2 menu tree - indexed by depth_1 position
const String **depth_2_menuTree[] PROGMEM = {
    depth_2_btJam,    // [0] Bluetooth Jam submenus
    depth_2_wifiJam,  // [1] WiFi Jam submenus
    nullptr,          // [2] BLE Jam - no submenus
    nullptr,          // [3] Zigbee Jam - no submenus
    depth_2_droneJam, // [4] Drone Jam submenus
    nullptr,          // [5] Misc Jam - no submenus
    depth_2_settings, // [6] Settings submenus
    nullptr           // [7] About - no submenus
};

// Pre-calculated menu sizes
const uint8_t depth_0_menuSize PROGMEM = sizeof(menu_main) / sizeof(menu_main[0]);

const uint8_t depth_1_menuSizes[] PROGMEM = {
    sizeof(menu_btJam) / sizeof(menu_btJam[0]),      // [0] Bluetooth Jam
    sizeof(menu_wifiJam) / sizeof(menu_wifiJam[0]),  // [1] WiFi Jam
    0,                                                // [2] BLE Jam - no submenu
    0,                                                // [3] Zigbee Jam - no submenu
    sizeof(menu_droneJam) / sizeof(menu_droneJam[0]),// [4] Drone Jam
    0,                                                // [5] Misc Jam - no submenu
    sizeof(menu_settings) / sizeof(menu_settings[0]),// [6] Settings
    0                                                 // [7] About - no submenu
};

const uint8_t depth_2_btJam_sizes[] PROGMEM = {0, 0, 0, 0}; // All nullptr
const uint8_t depth_2_wifiJam_sizes[] PROGMEM = {0, 0, 0}; // All nullptr
const uint8_t depth_2_droneJam_sizes[] PROGMEM = {0, 0, 0}; // All nullptr
const uint8_t depth_2_settings_sizes[] PROGMEM = {
    0,  // [0] Radios - nullptr
    sizeof(menu_jammingModes) / sizeof(menu_jammingModes[0]), // [1] Jamming Mode
    0,  // [2] Factory Reset - nullptr
    0   // [3] Return - nullptr
};

const uint8_t *depth_2_menuSizes[] PROGMEM = {
    depth_2_btJam_sizes,    // [0]
    depth_2_wifiJam_sizes,  // [1]
    nullptr,                // [2]
    nullptr,                // [3]
    depth_2_droneJam_sizes, // [4]
    nullptr,                // [5]
    depth_2_settings_sizes, // [6]
    nullptr                 // [7]
};

// Function pointer type for menu actions (returns true when action is complete)
typedef bool (*MenuAction)();

// Currently running action (executed each loop until it returns true)
static MenuAction currentAction = nullptr;

// Forward declarations for menu actions
bool action_notImplemented();
bool action_bleJam();
bool action_zigbeeJam();
bool action_miscJam();
bool action_about();
bool action_btChannelList();
bool action_btAllSequential();
bool action_btAllRandom();
bool action_wifiAllRandom();
bool action_wifiSingleChannel();
bool action_droneAllRandom();
bool action_droneAllSequential();
bool action_radiosConfig();
bool action_factorySettings();
bool action_jammingSimultaneous();
bool action_jammingStandalone();

// Menu action arrays (parallel to menu trees)
const MenuAction depth_0_actions[] = {
    nullptr,             // [0] Bluetooth Jam - has submenu
    nullptr,             // [1] WiFi Jam - has submenu
    action_bleJam,       // [2] BLE Jam - leaf action
    action_zigbeeJam,    // [3] Zigbee Jam - leaf action
    nullptr,             // [4] Drone Jam - has submenu
    action_miscJam,      // [5] Misc Jam - leaf action
    nullptr,             // [6] Settings - has submenu
    action_about         // [7] About - leaf action
};

const MenuAction depth_1_btJam_actions[] = {
    action_btChannelList,    // [0] Channel List (21)
    action_btAllSequential,  // [1] All channels (sequential)
    action_btAllRandom,      // [2] All channels (random)
    nullptr                  // [3] Return - handled separately
};

const MenuAction depth_1_wifiJam_actions[] = {
    action_wifiAllRandom,      // [0] All channels (random)
    action_wifiSingleChannel,  // [1] Single channel
    nullptr                    // [2] Return
};

const MenuAction depth_1_droneJam_actions[] = {
    action_droneAllRandom,     // [0] All channels (random)
    action_droneAllSequential, // [1] All channels (sequential)
    nullptr                    // [2] Return
};

const MenuAction depth_1_settings_actions[] = {
    action_radiosConfig, // [0] Radios
    nullptr,             // [1] Jamming Mode - has submenu
    action_factorySettings,    // [2] Factory Reset
    nullptr              // [3] Return
};

const MenuAction *depth_1_actions[] = {
    depth_1_btJam_actions,      // [0] Bluetooth Jam actions
    depth_1_wifiJam_actions,    // [1] WiFi Jam actions
    nullptr,                    // [2] BLE Jam - no submenu
    nullptr,                    // [3] Zigbee Jam - no submenu
    depth_1_droneJam_actions,   // [4] Drone Jam actions
    nullptr,                    // [5] Misc Jam - no submenu
    depth_1_settings_actions,   // [6] Settings actions
    nullptr                     // [7] About - no submenu
};

const MenuAction depth_2_settings_jammingMode_actions[] = {
    action_jammingSimultaneous, // [0] Simultaneous
    action_jammingStandalone    // [1] Standalone
};

// Note: Only settings->jamming mode has depth 2 actions
const MenuAction *depth_2_settings_actions[] = {
    nullptr,                             // [0] Radios - no submenu
    depth_2_settings_jammingMode_actions,// [1] Jamming Mode actions
    nullptr,                             // [2] Factory Reset - no submenu
    nullptr                              // [3] Return
};

const MenuAction **depth_2_actions[] = {
    nullptr,                    // [0] BT Jam - no depth 2 actions
    nullptr,                    // [1] WiFi Jam - no depth 2 actions
    nullptr,                    // [2] BLE Jam
    nullptr,                    // [3] Zigbee Jam
    nullptr,                    // [4] Drone Jam - no depth 2 actions
    nullptr,                    // [5] Misc Jam
    depth_2_settings_actions,   // [6] Settings has depth 2 actions
    nullptr                     // [7] About
};

bool uiRefresh = false;
uint8_t menuDepth = 0;
uint8_t menuPositions[3] = {0, 0, 0};

bool load_configs();
void error(const String &msg);
String get_battery_percentage();
void factory_settings();
bool settings_subMenu();

void setup() {
  Serial.begin(115200);
  Serial.printf("Log level: %d\n", ARDUHAL_LOG_LEVEL);
  display.begin();
  display.setRotation(2);
  ui.setDisplay(&display, displayWidth, displayHeight);
  ui.setTitleFont(&Picopixel);
  ui.setContentFont(&Picopixel);
  buttonOk.begin(PIN_OK, INPUT_PULLUP, true);
  buttonNext.begin(PIN_NEXT, INPUT_PULLUP, true);
  buttonPrev.begin(PIN_PREV, INPUT_PULLUP, true);

  if (buttonOk.isPressedRaw() && buttonPrev.isPressedRaw()) {
    factory_settings();
  }

  if (!load_configs()) {
    error("Failed to load configurations.");
  }

  display.clearDisplay();
  ui.showRunningActivity(*(bitmap_boot_logo.frames), bitmap_boot_logo.width, bitmap_boot_logo.height, "otg jammer");
  display.display();
  delay(2000);
  uiRefresh = true;
}

void loop() {
  buttonOk.loop();
  buttonNext.loop();
  buttonPrev.loop();
  ui.update();
  display.display();

  // If an action is active, execute it on every loop iteration.
  // When it reports completion, return to previous depth level.
  if (currentAction != nullptr) {
    bool done = currentAction();
    if (done) {
      currentAction = nullptr;
      if (menuDepth > 0) {
        menuDepth--;
        menuPositions[menuDepth + 1] = 0; // Reset child position
      }
      uiRefresh = true;
    }
    // Skip menu navigation while an action is active.
    return;
  }

  // Render current menu level
  if (uiRefresh) {
    uiRefresh = false;
    
    const String *currentMenu = nullptr;
    String currentTitle = "";
    uint8_t menuSize = 0;

    // Determine which menu to display based on current depth
    if (menuDepth == 0) {
      currentMenu = depth_0_menuTree;
      currentTitle = title_menu_main;
      menuSize = depth_0_menuSize;
    } 
    else if (menuDepth == 1) {
      currentMenu = depth_1_menuTree[menuPositions[0]];
      currentTitle = menu_main[menuPositions[0]];
      menuSize = depth_1_menuSizes[menuPositions[0]];
    }
    else if (menuDepth == 2) {
      if (depth_2_menuTree[menuPositions[0]] != nullptr) {
        currentMenu = depth_2_menuTree[menuPositions[0]][menuPositions[1]];
        if (currentMenu != nullptr) {
          currentTitle = depth_1_menuTree[menuPositions[0]][menuPositions[1]];
          menuSize = depth_2_menuSizes[menuPositions[0]][menuPositions[1]];
        }
      }
    }

    if (currentMenu != nullptr && menuSize > 0) {
      ui.optionSelectScreen(currentTitle, get_battery_percentage(), currentMenu, menuSize, menuPositions[menuDepth]);
    }
  }

  // Handle button input
  if (buttonNext.wasPressed()) {
    uint8_t menuSize = 0;
    if (menuDepth == 0) {
      menuSize = depth_0_menuSize;
    } else if (menuDepth == 1) {
      menuSize = depth_1_menuSizes[menuPositions[0]];
    } else if (menuDepth == 2 && depth_2_menuSizes[menuPositions[0]] != nullptr) {
      menuSize = depth_2_menuSizes[menuPositions[0]][menuPositions[1]];
    }
    
    if (menuSize > 0) {
      menuPositions[menuDepth] = (menuPositions[menuDepth] + 1) % menuSize;
    }
    buttonNext.resetPressedState();
    uiRefresh = true;
  }

  if (buttonPrev.wasPressed()) {
    uint8_t menuSize = 0;
    if (menuDepth == 0) {
      menuSize = depth_0_menuSize;
    } else if (menuDepth == 1) {
      menuSize = depth_1_menuSizes[menuPositions[0]];
    } else if (menuDepth == 2 && depth_2_menuSizes[menuPositions[0]] != nullptr) {
      menuSize = depth_2_menuSizes[menuPositions[0]][menuPositions[1]];
    }
    
    if (menuSize > 0) {
      menuPositions[menuDepth] = (menuPositions[menuDepth] - 1 + menuSize) % menuSize;
    }
    buttonPrev.resetPressedState();
    uiRefresh = true;
  }

  if (buttonOk.wasPressed()) {
    buttonOk.resetPressedState();
    
    // Check if "Return" option was selected (always last item)
    bool isReturn = false;
    if (menuDepth == 1) {
      uint8_t menuSize = depth_1_menuSizes[menuPositions[0]];
      if (menuSize > 0) {
        isReturn = (menuPositions[1] == menuSize - 1);
      }
    }

    if (isReturn) {
      // Go back to previous depth
      if (menuDepth > 0) {
        menuDepth--;
        menuPositions[menuDepth + 1] = 0; // Reset child position
        uiRefresh = true;
      }
    } else {
      // Try to enter submenu
      bool hasSubmenu = false;
      
      if (menuDepth == 0) {
        // Check if depth_1 has a submenu
        hasSubmenu = (depth_1_menuTree[menuPositions[0]] != nullptr);
      } else if (menuDepth == 1) {
        // Check if depth_2 has a submenu
        if (depth_2_menuTree[menuPositions[0]] != nullptr) {
          hasSubmenu = (depth_2_menuTree[menuPositions[0]][menuPositions[1]] != nullptr);
        }
      }

      if (hasSubmenu && menuDepth < 2) {
        // Enter submenu
        menuDepth++;
        menuPositions[menuDepth] = 0;
        uiRefresh = true;
      } else {
        // Leaf action - resolve and start executing on each loop tick
        MenuAction action = nullptr;

        if (menuDepth == 0) {
          action = depth_0_actions[menuPositions[0]];
        } else if (menuDepth == 1) {
          if (depth_1_actions[menuPositions[0]] != nullptr) {
            action = depth_1_actions[menuPositions[0]][menuPositions[1]];
          }
        } else if (menuDepth == 2) {
          if (depth_2_actions[menuPositions[0]] != nullptr &&
              depth_2_actions[menuPositions[0]][menuPositions[1]] != nullptr) {
            action = depth_2_actions[menuPositions[0]][menuPositions[1]][menuPositions[2]];
          }
        }

        // Start the action; it will be executed in subsequent loop iterations
        currentAction = (action != nullptr) ? action : action_notImplemented;
      }
    }
  }
}

bool action_notImplemented() {
  static uint32_t startTime = 0;
  static const uint32_t DURATION = 2000;
  
  // First call - initialize and display
  if (startTime == 0) {
    ui.runningActivityScreen("Not Implemented", get_battery_percentage(), *(bitmap_information_sign.frames),
                             bitmap_information_sign.width, bitmap_information_sign.height,
                             "This feature is not yet implemented.");
    display.display();
    startTime = millis();
    return false;  // Still running
  }
  
  // Check if duration elapsed
  if (millis() - startTime >= DURATION) {
    startTime = 0;  // Reset for next call
    return true;    // Action complete
  }
  
  return false;  // Still running
}

bool action_bleJam() {
  return action_notImplemented();
  // TODO: Implement BLE jamming logic
}

bool action_zigbeeJam() {
  return action_notImplemented();
  // TODO: Implement Zigbee jamming
}

bool action_miscJam() {
  return action_notImplemented();
  // TODO: Implement misc jamming
}

bool action_about() {
  static uint32_t startTime = 0;
  static const uint32_t DURATION = 3000;
  
  if (startTime == 0) {
    ui.runningActivityScreen("About", get_battery_percentage(), *(bitmap_information_sign.frames),
                             bitmap_information_sign.width, bitmap_information_sign.height,
                             "OTG Jammer v1.0\nby fpp3\n2026");
    display.display();
    startTime = millis();
    return false;
  }
  
  if (millis() - startTime >= DURATION) {
    startTime = 0;
    return true;
  }
  
  return false;
}

bool action_btChannelList() {
  return action_notImplemented();
  // TODO: Show Bluetooth channel list
}

bool action_btAllSequential() {
  return action_notImplemented();
  // TODO: Jam all BT channels sequentially
}

bool action_btAllRandom() {
  return action_notImplemented();
  // TODO: Jam all BT channels randomly
}

bool action_wifiAllRandom() {
  return action_notImplemented();
  // TODO: Jam WiFi channels randomly
}

bool action_wifiSingleChannel() {
  return action_notImplemented();
  // TODO: Jam single WiFi channel
}

bool action_droneAllRandom() {
  return action_notImplemented();
  // TODO: Jam drone frequencies randomly
}

bool action_droneAllSequential() {
  return action_notImplemented();
  // TODO: Jam drone frequencies sequentially
}

bool action_radiosConfig() {
  return action_notImplemented();
  // TODO: Configure radios
}

bool action_factorySettings() {
  // Factory settings has its own blocking logic for now
  factory_settings();
  return true;  // Return immediately after calling
}

bool action_jammingSimultaneous() {
  return action_notImplemented();
  // TODO: Set jamming mode to simultaneous
}

bool action_jammingStandalone() {
  return action_notImplemented();
  // TODO: Set jamming mode to standalone
}

// ============== Config & Helper Functions ==============

bool load_configs() {
  Preferences prefs;
  uint8_t numRadios;
  size_t radioConfigSize;
  radio_config_s *radiosConfig;
  size_t jamModeSize;
  jam_tx_mode_e jamMode;

  if (prefs.begin(CONFIG_NAMESPACE, true) == false) {
    log_d("Failed to open namespace %s!", CONFIG_NAMESPACE);
    return 0;
  } // Read-only mode
  for (uint8_t i = 0; CONFIG_MANDATORY_KEYS[i] != nullptr; i++) {
    if (!prefs.isKey(CONFIG_MANDATORY_KEYS[i])) {
      log_d("Missing mandatory namespace key %s!", CONFIG_MANDATORY_KEYS[i]);
      prefs.end();
      return 0;
    }
  }

  numRadios = prefs.getUChar(CONFIG_KEY_RADIO_NUM, 0);
  if (numRadios > 0) {
    radioConfigSize = prefs.getBytesLength(CONFIG_KEY_RADIO_CONFIG_STRUCT);
    radiosConfig = new radio_config_s[numRadios];
    if (radiosConfig == nullptr) {
      log_d("Could not allocate memory for radiosConfig struct");
      prefs.end();
      return 0;
    }
    size_t readBytes = prefs.getBytes(CONFIG_KEY_RADIO_CONFIG_STRUCT, radiosConfig, sizeof(radio_config_s) * numRadios);
    if (readBytes != radioConfigSize) {
      log_d("readBytes != radioConfigSize");
      prefs.end();
      return 0;
    }
  }
  jamModeSize = prefs.getBytesLength(CONFIG_KEY_RADIO_JAM_MODE);
  size_t readBytes = prefs.getBytes(CONFIG_KEY_RADIO_JAM_MODE, &jamMode, sizeof(jam_tx_mode_e));
  if (readBytes != jamModeSize) {
    log_d("readBytes != jamModeSize");
    prefs.end();
    return 0;
  }

  load_radios(radiosConfig, numRadios);
  set_jam_tx_mode(jamMode);

  prefs.end();
  log_d("Exit load_configs()");
  return true; // Assume success for this example
}

void error(const String &msg) {
  ui.runningActivityScreen("Error!", get_battery_percentage(), bitmap_error.frames, bitmap_error.frameCount,
                           bitmap_error.width, bitmap_error.height, bitmap_error.frameDurationMs, msg);
  while (1) {
    ui.update();
    display.display();
  }
}

String get_battery_percentage() { return String(map(analogRead(PIN_VBAT), VBAT_VOLTAGE_EMPTY, VBAT_VOLTAGE_FULL, 0, 100)) + "%"; }

void factory_settings() {
  ui.runningActivityScreen("Factory Reset", get_battery_percentage(), *(bitmap_information_sign.frames),
                           bitmap_information_sign.width, bitmap_information_sign.height, "Lift buttons to continue.");
  display.display();
  while (buttonOk.isPressedRaw() || buttonPrev.isPressedRaw())
    ;
  String options[] = {"Load", "Cancel"};
  uint8_t pos = 1;
  buttonOk.resetPressedState();
  buttonNext.resetPressedState();
  buttonPrev.resetPressedState();
  while (buttonOk.wasPressed() == false) {
    buttonOk.loop();
    buttonNext.loop();
    buttonPrev.loop();
    if (buttonNext.wasPressed()) {
      pos = (pos + 1) % 2;
      log_d("buttonNext was pressed");
      buttonNext.resetPressedState();
    }
    if (buttonPrev.wasPressed()) {
      pos = (pos - 1) % 2;
      log_d("buttonPrev was pressed");
      buttonPrev.resetPressedState();
    }
    ui.confirmScreen("Factory Reset", get_battery_percentage(), *(bitmap_device_reset.frames),
                     bitmap_device_reset.width, bitmap_device_reset.height, "Load factory Defaults?", options, 2, pos);
    display.display();
  }
  if (pos == 0) {
    log_d("Load factory settings");
    Preferences prefs;
    prefs.begin(CONFIG_NAMESPACE, false); // Read-write mode
    prefs.clear();
    prefs.putUChar(CONFIG_KEY_RADIO_NUM, 0);
    jam_tx_mode_e defaultJamMode = JAM_TX_SIMULTANEOUS;
    prefs.putBytes(CONFIG_KEY_RADIO_JAM_MODE, &defaultJamMode, sizeof(jam_tx_mode_e));
    prefs.end();
    ui.runningActivityScreen("Factory Reset", get_battery_percentage(), *(bitmap_check.frames), bitmap_check.width,
                             bitmap_check.height, "Factory settings loaded.");
    display.display();
    delay(2000);
    ESP.restart();
  }
  buttonOk.resetPressedState();
  buttonNext.resetPressedState();
  buttonPrev.resetPressedState();
  log_d("Exit factory_settings()");
}
