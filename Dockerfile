FROM ghcr.io/wiiu-env/devkitppc:20230326

COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20230316 /artifacts $DEVKITPRO

WORKDIR project
