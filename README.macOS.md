# Developing on macOS

Install Cmake and SDL2. (If you don't have [Homebrew](https://brew.sh/), get that first.)

```bash
brew install cmake sdl2
```

Generate and open an Xcode project.

```bash
cmake -G Xcode .
open Aerofoil.xcodeproj
```

Select the `AerofoilX` target in Xcode and build.

## Current status

Compiles but not even creating a window yet. `AddInstanceForFactory` looks like it's AVFoundation-related, but might not be the root cause.

```
2021-03-30 18:24:47.419584-0700 AerofoilX[5744:5185611] Metal API Validation Enabled
2021-03-30 18:24:47.447837-0700 AerofoilX[5744:5186044] flock failed to lock maps file: errno = 35
2021-03-30 18:24:47.448157-0700 AerofoilX[5744:5186044] flock failed to lock maps file: errno = 35
2021-03-30 18:24:47.509443-0700 AerofoilX[5744:5185611] [plugin] AddInstanceForFactory: No factory registered for id <CFUUID 0x100c12790> F8BB1C28-BAE8-11D6-9C31-00039315CD46
Program ended with exit code: 0
```

Changes not tested to see if they break other platforms yet.