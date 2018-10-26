# Padcon Plugin for the Wii U Plugin System
This plugin for WUPS turns the GamePad screen off, but keeps it on as a controller. Modified version of the [Padcon plugin](https://github.com/Maschell/WUPSPluginPlayground/tree/master/padcon) by Maschell. I just cleaned it up and changed the button combination.

## Wii U Plugin System
This is a plugin for the [Wii U Plugin System (WUPS)](https://github.com/Maschell/WiiUPluginSystem/). To be able to use this plugin you have to place the resulting `.mod` file into the following folder:

```
sd:/wiiu/plugins
```
When the file is placed on the SDCard you can load it with [plugin loader](https://github.com/Maschell/WiiUPluginSystem/).

## Using
While the plugin is active, press ZL+ZR+L+R+Plus together to turn off the screen. The GamePad will still be usable as a controller.

## Building
For building you need:
- [wups](https://github.com/Maschell/WiiUPluginSystem)
- [wut](https://github.com/decaf-emu/wut)
- [libutilswut](https://github.com/Maschell/libutils/tree/wut) (WUT version) for common functions.

Install them (in this order) according to their README's. Don't forget the dependencies of the libs itself.
