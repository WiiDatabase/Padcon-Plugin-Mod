#include "main.h"
#include "utils/WUPSConfigItemButtonCombo.h"
#include "utils/logger.h"
#include <nn/ccr/sys.h>
#include <string_view>
#include <vpad/input.h>
#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>

WUPS_PLUGIN_NAME("Padcon");
WUPS_PLUGIN_DESCRIPTION("Turns the gamepad screen on/off or shuts the GamePad off");
WUPS_PLUGIN_VERSION(VERSION_FULL);
WUPS_PLUGIN_AUTHOR("WiiDatabase.de");
WUPS_PLUGIN_LICENSE("GPLv3");

WUPS_USE_STORAGE("padcon_plugin");

#define BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING "buttonComboDisableScreen"
#define BUTTON_COMBO_SHUT_OFF_CONFIG_STRING       "buttonComboShutOff"
#define ENABLED_CONFIG_STRING                     "enabled"


uint8_t cooldown                  = 0;
bool enabled                      = true;
uint32_t buttonComboDisableScreen = (VPAD_BUTTON_ZL | VPAD_BUTTON_ZR | VPAD_BUTTON_L |
                                     VPAD_BUTTON_R | VPAD_BUTTON_PLUS);
uint32_t buttonComboShutOff       = (VPAD_BUTTON_ZL | VPAD_BUTTON_ZR | VPAD_BUTTON_L |
                               VPAD_BUTTON_R | VPAD_BUTTON_MINUS);

void blockTvButton() {
    if (!enabled) {
        DEBUG_FUNCTION_LINE("Unblock TV Menu");
        VPADSetTVMenuInvalid(VPAD_CHAN_0, false);
        return;
    }

    if (buttonComboDisableScreen & VPAD_BUTTON_TV || buttonComboShutOff & VPAD_BUTTON_TV) {
        DEBUG_FUNCTION_LINE("Block TV Menu");
        VPADSetTVMenuInvalid(VPAD_CHAN_0, true);
    } else {
        DEBUG_FUNCTION_LINE("Unblock TV Menu");
        VPADSetTVMenuInvalid(VPAD_CHAN_0, false);
    }
}

INITIALIZE_PLUGIN() {
    initLogging();
    DEBUG_FUNCTION_LINE("init plugin");

    // Open storage to read values
    WUPSStorageError storageRes = WUPS_OpenStorage();
    if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to open storage %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
    } else {
        // Try to get value from storage
        if ((storageRes = WUPS_GetBool(nullptr, ENABLED_CONFIG_STRING, &enabled)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            // Add the value to the storage if it's missing.
            if (WUPS_StoreBool(nullptr, ENABLED_CONFIG_STRING, enabled) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("Failed to store value");
            }
        } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to get value %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        // Try to get value from storage
        if ((storageRes = WUPS_GetInt(nullptr, BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING, reinterpret_cast<int32_t *>(&buttonComboDisableScreen))) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            // Add the value to the storage if it's missing.
            if (WUPS_StoreInt(nullptr, BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING, (int32_t) buttonComboDisableScreen) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("Failed to store value");
            }
        } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to get value %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        // Try to get value from storage
        if ((storageRes = WUPS_GetInt(nullptr, BUTTON_COMBO_SHUT_OFF_CONFIG_STRING, reinterpret_cast<int32_t *>(&buttonComboShutOff))) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            // Add the value to the storage if it's missing.
            if (WUPS_StoreInt(nullptr, BUTTON_COMBO_SHUT_OFF_CONFIG_STRING, (int32_t) buttonComboShutOff) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("Failed to store value");
            }
        } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to get value %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        // Close storage
        if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to close storage");
        }
    }

    blockTvButton();

    deinitLogging();
}

void buttonComboItemChanged(ConfigItemButtonCombo *item, uint32_t newValue) {
    if (item && item->configId) {
        DEBUG_FUNCTION_LINE("New value in %s changed: %d", item->configId, newValue);
        if (std::string_view(item->configId) == BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING) {
            buttonComboDisableScreen = newValue;
            WUPS_StoreInt(nullptr, item->configId, (int32_t) buttonComboDisableScreen);
        } else if (std::string_view(item->configId) == BUTTON_COMBO_SHUT_OFF_CONFIG_STRING) {
            buttonComboShutOff = newValue;
            WUPS_StoreInt(nullptr, item->configId, (int32_t) buttonComboShutOff);
        }
    }
}

void boolItemCallback(ConfigItemBoolean *item, bool newValue) {
    if (item && item->configId) {
        DEBUG_FUNCTION_LINE("New value in %s changed: %d", item->configId, newValue);
        if (std::string_view(item->configId) == ENABLED_CONFIG_STRING) {
            enabled = newValue;
            WUPS_StoreBool(nullptr, item->configId, enabled);
        }
    }
}

WUPS_CONFIG_CLOSED() {
    // Save all changes
    if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to close storage");
    }
    blockTvButton();
}

ON_APPLICATION_START() {
    initLogging();
}

ON_APPLICATION_ENDS() {
    deinitLogging();
}

WUPS_GET_CONFIG() {
    // We open the storage, so we can persist the configuration the user did.
    if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to open storage");
        return 0;
    }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "Padcon");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Settings", &cat);

    WUPSConfigItemBoolean_AddToCategoryHandled(
            config, cat, ENABLED_CONFIG_STRING, "Enable plugin", enabled,
            &boolItemCallback);

    WUPSConfigItemButtonCombo_AddToCategoryHandled(config, cat, BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING, "Disable screen", buttonComboDisableScreen, &buttonComboItemChanged);

    WUPSConfigItemButtonCombo_AddToCategoryHandled(config, cat, BUTTON_COMBO_SHUT_OFF_CONFIG_STRING, "Shut off", buttonComboShutOff, &buttonComboItemChanged);


    return config;
}

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan, VPADStatus *buffer,
              uint32_t buffer_size, VPADReadError *error) {
    int32_t result = real_VPADRead(chan, buffer, buffer_size, error);

    if (!enabled) {
        return result;
    }

    if (result > 0) {
        if (error && *error != VPAD_READ_SUCCESS) {
            return result;
        }
        if (cooldown == 0) {
            cooldown = 60; // 1 second cooldown

            uint32_t hold = buffer[0].hold;
            VPADLcdMode lcdMode;

            if (hold == buttonComboDisableScreen) {
                VPADGetLcdMode(VPAD_CHAN_0, &lcdMode);

                if (lcdMode == VPAD_LCD_ON) {
                    DEBUG_FUNCTION_LINE("Turning off GamePad screen");
                    VPADSetLcdMode(VPAD_CHAN_0, VPAD_LCD_OFF);
                } else {
                    DEBUG_FUNCTION_LINE("Turning on GamePad screen");
                    VPADSetLcdMode(VPAD_CHAN_0, VPAD_LCD_ON);
                }
            } else if (hold == buttonComboShutOff) {
                VPADGetLcdMode(VPAD_CHAN_0, &lcdMode);

                int32_t ret = 0;

                if (lcdMode == VPAD_LCD_OFF) {
                    DEBUG_FUNCTION_LINE("Shutting off GamePad");
                    ret = VPADSetLcdMode(VPAD_CHAN_0, VPAD_LCD_ON);
                }

                if (ret == 0) {
                    CCRSysDRCShutdown();
                }
            }
        }
    }

    if (cooldown > 0) {
        cooldown--;
    }
    return result;
}

WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);
