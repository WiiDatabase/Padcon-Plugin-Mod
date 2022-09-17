#include <nn/ccr/sys.h>
#include <vpad/input.h>
#include <wups.h>

WUPS_PLUGIN_NAME("Padcon");
WUPS_PLUGIN_DESCRIPTION("Turns the gamepad screen on/off or shuts the GamePad "
                        "off when pressing ZL + ZR + L + R + Plus/Minus.");
WUPS_PLUGIN_VERSION("v1.2.2-Mod");
WUPS_PLUGIN_AUTHOR("Maschell, Brawl");
WUPS_PLUGIN_LICENSE("GPLv3");

uint8_t cooldown = 0;

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan, VPADStatus *buffer,
              uint32_t buffer_size, VPADReadError *error) {
  int32_t result = real_VPADRead(chan, buffer, buffer_size, error);

  if (result > 0) {
    if (error && *error != VPAD_READ_SUCCESS) {
      return result;
    }
    if (cooldown == 0) {
      cooldown = 60; // 1 second cooldown

      uint32_t hold = buffer[0].hold;
      VPADLcdMode lcdMode;

      if (hold == (VPAD_BUTTON_ZL | VPAD_BUTTON_ZR | VPAD_BUTTON_L |
                   VPAD_BUTTON_R | VPAD_BUTTON_PLUS)) {
        VPADGetLcdMode(VPAD_CHAN_0, &lcdMode);

        if (lcdMode == VPAD_LCD_ON) {
          VPADSetLcdMode(VPAD_CHAN_0, VPAD_LCD_OFF);
        } else {
          VPADSetLcdMode(VPAD_CHAN_0, VPAD_LCD_ON);
        }
      } else if (hold == (VPAD_BUTTON_ZL | VPAD_BUTTON_ZR | VPAD_BUTTON_L |
                          VPAD_BUTTON_R | VPAD_BUTTON_MINUS)) {
        VPADGetLcdMode(VPAD_CHAN_0, &lcdMode);

        int32_t ret = 0;

        if (lcdMode == VPAD_LCD_OFF) {
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
