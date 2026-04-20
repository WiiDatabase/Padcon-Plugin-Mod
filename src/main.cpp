#include "main.h"

#include "utils/StringTools.h"
#include "utils/logger.h"

#include <forward_list>
#include <nn/ccr/sys.h>
#include <notifications/notifications.h>
#include <string_view>
#include <vpad/input.h>
#include <wups.h>
#include <wups/button_combo/api.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemButtonCombo.h>

#define PLUGIN_NAME "Padcon"

WUPS_PLUGIN_NAME(PLUGIN_NAME);
WUPS_PLUGIN_DESCRIPTION("Turns the gamepad screen on/off or shuts the GamePad off");
WUPS_PLUGIN_VERSION(VERSION_FULL);
WUPS_PLUGIN_AUTHOR("WiiDatabase.de");
WUPS_PLUGIN_LICENSE("GPLv3");

WUPS_USE_STORAGE("padcon_plugin");

#define BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING_DEPRECATED "buttonComboDisableScreen"
#define BUTTON_COMBO_SHUT_OFF_CONFIG_STRING_DEPRECATED       "buttonComboShutOff"

#define BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING            "buttonComboDisableScreenV2"
#define BUTTON_COMBO_SHUT_OFF_CONFIG_STRING                  "buttonComboShutOffV2"
#define ENABLED_CONFIG_STRING                                "enabled"

constexpr bool CONFIG_ENABLED_DEFAULT                                        = true;
constexpr WUPSButtonCombo_Buttons CONFIG_BUTTON_COMBO_DISABLE_SCREEN_DEFAULT = (WUPS_BUTTON_COMBO_BUTTON_ZL | WUPS_BUTTON_COMBO_BUTTON_ZR | WUPS_BUTTON_COMBO_BUTTON_L | WUPS_BUTTON_COMBO_BUTTON_R | WUPS_BUTTON_COMBO_BUTTON_PLUS);
constexpr WUPSButtonCombo_Buttons CONFIG_BUTTON_COMBO_SHUT_OFF_DEFAULT       = (WUPS_BUTTON_COMBO_BUTTON_ZL | WUPS_BUTTON_COMBO_BUTTON_ZR | WUPS_BUTTON_COMBO_BUTTON_L | WUPS_BUTTON_COMBO_BUTTON_R | WUPS_BUTTON_COMBO_BUTTON_MINUS);

static bool sEnabled                                     = CONFIG_ENABLED_DEFAULT;
static WUPSButtonCombo_Buttons sButtonComboDisableScreen = CONFIG_BUTTON_COMBO_DISABLE_SCREEN_DEFAULT;
static WUPSButtonCombo_Buttons sButtonComboShutOff       = CONFIG_BUTTON_COMBO_SHUT_OFF_DEFAULT;

static std::forward_list<WUPSButtonComboAPI::ButtonCombo> sButtonComboInstances;
static WUPSButtonCombo_ComboHandle sDisableScreenHandle(nullptr);
static WUPSButtonCombo_ComboHandle sShutOffHandle(nullptr);
static bool sNotificationModuleInitDone = false;


uint32_t migrateButtonCombo(const uint32_t buttons) {
    uint32_t conv_buttons = 0;

    if (buttons & VPAD_BUTTON_A) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_A;
    }
    if (buttons & VPAD_BUTTON_B) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_B;
    }
    if (buttons & VPAD_BUTTON_X) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_X;
    }
    if (buttons & VPAD_BUTTON_Y) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_Y;
    }

    if (buttons & VPAD_BUTTON_LEFT) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_LEFT;
    }
    if (buttons & VPAD_BUTTON_RIGHT) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_RIGHT;
    }
    if (buttons & VPAD_BUTTON_UP) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_UP;
    }
    if (buttons & VPAD_BUTTON_DOWN) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_DOWN;
    }

    if (buttons & VPAD_BUTTON_ZL) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_ZL;
    }
    if (buttons & VPAD_BUTTON_ZR) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_ZR;
    }

    if (buttons & VPAD_BUTTON_L) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_L;
    }
    if (buttons & VPAD_BUTTON_R) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_R;
    }

    if (buttons & VPAD_BUTTON_PLUS) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_PLUS;
    }
    if (buttons & VPAD_BUTTON_MINUS) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_MINUS;
    }

    if (buttons & VPAD_BUTTON_STICK_R) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_STICK_R;
    }
    if (buttons & VPAD_BUTTON_STICK_L) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_STICK_L;
    }

    if (buttons & VPAD_BUTTON_TV) {
        conv_buttons |= WUPS_BUTTON_COMBO_BUTTON_TV;
    }

    return conv_buttons;
}

void blockTvButton() {
    if (!sEnabled) {
        DEBUG_FUNCTION_LINE("Unblock TV Menu");
        VPADSetTVMenuInvalid(VPAD_CHAN_0, false);
        return;
    }

    if (sButtonComboDisableScreen & WUPS_BUTTON_COMBO_BUTTON_TV || sButtonComboShutOff & WUPS_BUTTON_COMBO_BUTTON_TV) {
        DEBUG_FUNCTION_LINE("Block TV Menu");
        VPADSetTVMenuInvalid(VPAD_CHAN_0, true);
    } else {
        DEBUG_FUNCTION_LINE("Unblock TV Menu");
        VPADSetTVMenuInvalid(VPAD_CHAN_0, false);
    }
}

void buttonComboItemChanged(ConfigItemButtonCombo *item, uint32_t newValue) {
    if (item && item->identifier) {
        DEBUG_FUNCTION_LINE("New value in %s changed: %d", item->identifier, newValue);
        if (std::string_view(item->identifier) == BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING) {
            sButtonComboDisableScreen = static_cast<WUPSButtonCombo_Buttons>(newValue);
            WUPSStorageAPI::Store(item->identifier, sButtonComboDisableScreen);
        } else if (std::string_view(item->identifier) == BUTTON_COMBO_SHUT_OFF_CONFIG_STRING) {
            sButtonComboShutOff = static_cast<WUPSButtonCombo_Buttons>(newValue);
            WUPSStorageAPI::Store(item->identifier, sButtonComboShutOff);
        }
    }
}

void boolItemCallback(ConfigItemBoolean *item, bool newValue) {
    if (item && item->identifier) {
        DEBUG_FUNCTION_LINE("New value in %s changed: %d", item->identifier, newValue);
        if (std::string_view(item->identifier) == ENABLED_CONFIG_STRING) {
            sEnabled = newValue;
            WUPSStorageAPI::Store(item->identifier, sEnabled);
        }
    }
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    try {
        auto settingsCat = WUPSConfigCategory::Create("Settings");

        settingsCat.add(WUPSConfigItemBoolean::Create(ENABLED_CONFIG_STRING, "Enable plugin",
                                                      CONFIG_ENABLED_DEFAULT,
                                                      sEnabled,
                                                      boolItemCallback));

        settingsCat.add(WUPSConfigItemButtonCombo::Create(BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING, "Disable screen",
                                                          CONFIG_BUTTON_COMBO_DISABLE_SCREEN_DEFAULT,
                                                          sDisableScreenHandle,
                                                          buttonComboItemChanged));

        settingsCat.add(WUPSConfigItemButtonCombo::Create(BUTTON_COMBO_SHUT_OFF_CONFIG_STRING, "Shut off",
                                                          CONFIG_BUTTON_COMBO_SHUT_OFF_DEFAULT,
                                                          sShutOffHandle,
                                                          buttonComboItemChanged));

        root.add(std::move(settingsCat));
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_ERR("Creating config menu failed: %s", e.what());
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();
}

WUPSButtonCombo_ComboHandle RegisterButtonCombo(const std::string_view label, const WUPSButtonCombo_Buttons buttonCombo, const WUPSButtonCombo_ComboCallback callback) {
    const auto buttonComboLabel        = string_format(PLUGIN_NAME ": %s", label.data());
    WUPSButtonCombo_ComboStatus status = WUPS_BUTTON_COMBO_COMBO_STATUS_INVALID_STATUS;
    WUPSButtonCombo_Error err          = WUPS_BUTTON_COMBO_ERROR_UNKNOWN_ERROR;
    auto res                           = WUPSButtonComboAPI::CreateComboPressDown(buttonComboLabel,
                                                                                  buttonCombo,
                                                                                  callback,
                                                                                  nullptr,
                                                                                  status,
                                                                                  err);
    if (!res || err != WUPS_BUTTON_COMBO_ERROR_SUCCESS) {
        const std::string errorMsg = string_format(PLUGIN_NAME ": Failed to register button combo \"%s\"", label.data());
        DEBUG_FUNCTION_LINE_ERR("%s", errorMsg.c_str());
        if (sNotificationModuleInitDone) {
            NotificationModule_AddErrorNotification(errorMsg.c_str());
        }
    } else {
        if (status == WUPS_BUTTON_COMBO_COMBO_STATUS_CONFLICT) {
            const auto conflictMsg = string_format(PLUGIN_NAME ": \"%s\"-combo was disabled due to a conflict. Please assign a different combo", label.data());
            DEBUG_FUNCTION_LINE_INFO("%s", conflictMsg.c_str());
            if (sNotificationModuleInitDone) {
                NotificationModule_AddInfoNotification(conflictMsg.c_str());
            }
        } else if (status != WUPS_BUTTON_COMBO_COMBO_STATUS_VALID) {
            const auto conflictMsg = string_format(PLUGIN_NAME ": Unknown error happened while registering button combo \"%s\"", label.data());
            DEBUG_FUNCTION_LINE_INFO("%s", conflictMsg.c_str());
            if (sNotificationModuleInitDone) {
                NotificationModule_AddInfoNotification(conflictMsg.c_str());
            }
        }
        const auto handle = res->getHandle();
        sButtonComboInstances.emplace_front(std::move(*res));
        return handle;
    }
    return WUPSButtonCombo_ComboHandle(nullptr);
}

void ScreenOffCallback(const WUPSButtonCombo_ControllerTypes triggeredBy, WUPSButtonCombo_ComboHandle, void *) {
    if (!sEnabled) {
        return;
    }
    VPADLcdMode lcdMode;
    VPADGetLcdMode(VPAD_CHAN_0, &lcdMode);

    if (lcdMode == VPAD_LCD_ON) {
        DEBUG_FUNCTION_LINE_INFO("Turning off GamePad screen");
        VPADSetLcdMode(VPAD_CHAN_0, VPAD_LCD_OFF);
    } else {
        DEBUG_FUNCTION_LINE_INFO("Turning on GamePad screen");
        VPADSetLcdMode(VPAD_CHAN_0, VPAD_LCD_ON);
    }
}

void ShutOffCallback(const WUPSButtonCombo_ControllerTypes, WUPSButtonCombo_ComboHandle, void *) {
    if (!sEnabled) {
        return;
    }
    VPADLcdMode lcdMode;
    VPADGetLcdMode(VPAD_CHAN_0, &lcdMode);

    int32_t ret = 0;

    if (lcdMode == VPAD_LCD_OFF) {
        DEBUG_FUNCTION_LINE_INFO("Turning on GamePad screen");
        ret = VPADSetLcdMode(VPAD_CHAN_0, VPAD_LCD_ON);
    }

    if (ret == 0) {
        DEBUG_FUNCTION_LINE_INFO("Shutting off GamePad");
        CCRSysDRCShutdown();
    }
}

void migrateStorage() {
    uint32_t oldButtonCombo = 0;
    if (WUPSStorageAPI::Get(BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING_DEPRECATED, oldButtonCombo) == WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_INFO("Found deprecated config in storage. Storage will be migrated");
        sButtonComboDisableScreen = static_cast<WUPSButtonCombo_Buttons>(migrateButtonCombo(oldButtonCombo));
        if (WUPSStorageAPI::DeleteItem(BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING_DEPRECATED) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to delete deprecated value: \"%s\" from storage", BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING_DEPRECATED);
        }
    }
    if (WUPSStorageAPI::Get(BUTTON_COMBO_SHUT_OFF_CONFIG_STRING_DEPRECATED, oldButtonCombo) == WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_INFO("Found deprecated config in storage. Storage will be migrated");
        sButtonComboShutOff = static_cast<WUPSButtonCombo_Buttons>(migrateButtonCombo(oldButtonCombo));
        if (WUPSStorageAPI::DeleteItem(BUTTON_COMBO_SHUT_OFF_CONFIG_STRING_DEPRECATED) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to delete deprecated value: \"%s\" from storage", BUTTON_COMBO_SHUT_OFF_CONFIG_STRING_DEPRECATED);
        }
    }
}

INITIALIZE_PLUGIN() {
    initLogging();

    if (NotificationModule_InitLibrary() == NOTIFICATION_MODULE_RESULT_SUCCESS) {
        sNotificationModuleInitDone = true;
    } else {
        sNotificationModuleInitDone = false;
        DEBUG_FUNCTION_LINE_ERR("Failed to init notification lib");
    }


    WUPSConfigAPIOptionsV1 configOptions = {.name = PLUGIN_NAME};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init config api");
    }

    sButtonComboDisableScreen = CONFIG_BUTTON_COMBO_DISABLE_SCREEN_DEFAULT; // migrateStorage might override this
    sButtonComboShutOff       = CONFIG_BUTTON_COMBO_SHUT_OFF_DEFAULT;       // migrateStorage might override this
    // migrate old button combos
    migrateStorage();

    WUPSStorageError storageRes;
    if ((storageRes = WUPSStorageAPI::GetOrStoreDefault(ENABLED_CONFIG_STRING, sEnabled, CONFIG_ENABLED_DEFAULT)) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }
    if ((storageRes = WUPSStorageAPI::GetOrStoreDefault(BUTTON_COMBO_DISABLE_SCREEN_CONFIG_STRING, sButtonComboDisableScreen, CONFIG_BUTTON_COMBO_DISABLE_SCREEN_DEFAULT)) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }
    if ((storageRes = WUPSStorageAPI::GetOrStoreDefault(BUTTON_COMBO_SHUT_OFF_CONFIG_STRING, sButtonComboShutOff, CONFIG_BUTTON_COMBO_SHUT_OFF_DEFAULT)) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }

    if ((storageRes = WUPSStorageAPI::SaveStorage()) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }

    sDisableScreenHandle = RegisterButtonCombo("Disable screen", sButtonComboDisableScreen, ScreenOffCallback);
    sShutOffHandle       = RegisterButtonCombo("Shut off", sButtonComboShutOff, ShutOffCallback);

    blockTvButton();

    deinitLogging();
}

ON_APPLICATION_START() {
    initLogging();
}

ON_APPLICATION_ENDS() {
    deinitLogging();
}
