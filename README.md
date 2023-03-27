# Padcon Plugin for the Wii U Plugin System

This plugin for the Wii U Plugin System turns the Wii U GamePad screen off, but keeps it on as a controller. Alternatively, it can also be turned off completely. Button combos can be adjusted in the config menu.

Modified version of the [Padcon plugin](https://github.com/Maschell/WUPSPluginPlayground/tree/master/padcon) by Maschell.

## Installation

1. Install Aroma
2. Check the Releases page and download the [latest non-preview version](https://github.com/WiiDatabase/Padcon-Plugin-Mod/releases/latest) of this plugin
3. Extract it to "`SD://wiiu/environments/aroma/plugins`"
4. Done!

## Usage

**You have to HOLD the buttons for a second, not just click them!**

| Default Button Combo    | Action                                                                  |
| ----------------------- | ----------------------------------------------------------------------- |
| ZL + ZR + L + R + Plus  | Disable GamePad screen, use as controller (press again to enable again) |
| ZL + ZR + L + R + Minus | Power the GamePad down completely (need to power it on again to use it) |

You can change these button combos in the WUPS Config Menu (L + D-Pad Down + Minus). The plugin can also be disabled there.

## Building

For building you need:

- [wups](https://github.com/Maschell/WiiUPluginSystem)
- [wut](https://github.com/devkitpro/wut)

Install them (in this order) according to their README's. Don't forget the dependencies of the libs itself.

Then you should be able to compile via `make` (with no logging) or `make DEBUG=1` (with logging).

## Buildflags

### Logging

Building via `make` only logs errors (via OSReport). To enable logging via the [LoggingModule](https://github.com/wiiu-env/LoggingModule) set `DEBUG` to `1` or `VERBOSE`.

`make` Logs errors only (via OSReport).  
`make DEBUG=1` Enables information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).  
`make DEBUG=VERBOSE` Enables verbose information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).

If the [LoggingModule](https://github.com/wiiu-env/LoggingModule) is not present, it'll fallback to UDP (Port 4405) and [CafeOS](https://github.com/wiiu-env/USBSerialLoggingModule) logging.

## Building using the Dockerfile

It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once)
docker build . -t wiiu-plugin-builder

# make
docker run -it --rm -v ${PWD}:/project wiiu-plugin-builder make DEBUG=1

# make clean
docker run -it --rm -v ${PWD}:/project wiiu-plugin-builder make clean
```

## Format the code via docker

`docker run --rm -v ${PWD}:/src ghcr.io/wiiu-env/clang-format:13.0.0-2 -r ./src -i`
