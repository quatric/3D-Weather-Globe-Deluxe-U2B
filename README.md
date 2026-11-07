# 3D Weather Globe Deluxe 2.0.7-U2B

Version-patched packaging for 3D Weather Globe Deluxe. This repository contains the Objective-C source translation (`src/`, `tools/`, `Makefile`) plus the patched `Info.plist` (version `2.0.7-U2B`), DMG layout settings and a build script. It does not contain the original application, its resources or its installer assets; supply your own copy of the app.

You can download the compiled DMG on MacSerialJunkie or MacBB.

    ./build.sh "/path/to/3D Weather Globe Deluxe Edition.app"

## Source

`src/` is a translation of the original program into Objective-C that builds as a Universal 2 (arm64 + x86_64) app for current macOS, produced from the original binary's Objective-C metadata and a Ghidra decompilation. `tools/` is the pipeline that generates and checks it, and the `Makefile` builds it. The original app (nibs, maps, atlas database, clouds) is needed as input and is not included; unpack your own copy into `game/`.

    make progress    # translated vs. remaining methods
    make -j8         # build the app
