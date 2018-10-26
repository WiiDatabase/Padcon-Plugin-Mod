#include <wups.h>
#include <string.h>

#include <nsysnet/socket.h>
#include <vpad/input.h>
#include <utils/logger.h>

WUPS_PLUGIN_NAME("Padcon");
WUPS_PLUGIN_DESCRIPTION("Turns the gamepad screen on/off when ZL+ZR+L+R+Plus.");
WUPS_PLUGIN_VERSION("v1.0-Mod");
WUPS_PLUGIN_AUTHOR("Maschell, Brawl");
WUPS_PLUGIN_LICENSE("GPL");

ON_APPLICATION_START(args){
   socket_lib_init();
   log_init();

   DEBUG_FUNCTION_LINE("Init of padcon!\n");
}

uint8_t gCallbackCooldown = 0;

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan, VPADStatus *buffer, uint32_t buffer_size, VPADReadError *error) {
    int32_t result = real_VPADRead(chan, buffer, buffer_size, error);
    if(result > 0 && *error == VPAD_READ_SUCCESS) {
        if(buffer[0].hold == (VPAD_BUTTON_ZL | VPAD_BUTTON_ZR | VPAD_BUTTON_L | VPAD_BUTTON_R | VPAD_BUTTON_PLUS) && gCallbackCooldown == 0) {
            gCallbackCooldown = 0x3C;
            VPADLcdMode mode;
            VPADGetLcdMode(0, (VPADLcdMode*)&mode);       // Get current display mode
            if(mode != 1) {
                VPADSetLcdMode(0, 1);       // Turn it off
            }
            else {
                VPADSetLcdMode(0, 0xFF);    // Turn it on
            }
        }
    }
    if(gCallbackCooldown > 0) gCallbackCooldown--;
    return result;
}

WUPS_MUST_REPLACE(VPADRead ,WUPS_LOADER_LIBRARY_VPAD, VPADRead);
