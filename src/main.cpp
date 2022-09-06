#include <vpad/input.h>
#include <wups.h>

WUPS_PLUGIN_NAME("Padcon");
WUPS_PLUGIN_DESCRIPTION(
    "Turns the gamepad screen on/off when ZL + ZR + L + R + Plus.");
WUPS_PLUGIN_VERSION("v1.1-Mod");
WUPS_PLUGIN_AUTHOR("Maschell, Brawl");
WUPS_PLUGIN_LICENSE("GPL");

uint8_t cooldown = 0;

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan, VPADStatus *buffer,
              uint32_t buffer_size, VPADReadError *error) {
  int32_t result = real_VPADRead(chan, buffer, buffer_size, error);

  if (result > 0 && *error == VPAD_READ_SUCCESS) {
    if (buffer[0].hold == (VPAD_BUTTON_ZL | VPAD_BUTTON_ZR | VPAD_BUTTON_L |
                           VPAD_BUTTON_R | VPAD_BUTTON_PLUS) &&
        cooldown == 0) {
      cooldown = 60; // 1 second cooldown
      VPADLcdMode lcdMode;
      VPADGetLcdMode(VPAD_CHAN_0, &lcdMode);

      if (lcdMode == VPAD_LCD_ON) {
        VPADSetLcdMode(VPAD_CHAN_0, VPAD_LCD_OFF);
      } else {
        VPADSetLcdMode(VPAD_CHAN_0, VPAD_LCD_ON);
      }
    }
  }

  if (cooldown > 0) {
    cooldown--;
  }
  return result;
}

WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);
