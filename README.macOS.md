# Developing on macOS

Install SDL2. (If you don't have [Homebrew](https://brew.sh/), get that first.)

```bash
brew install sdl2
```

Generate and open an Xcode project.

```bash
cmake -G Xcode .
open Aerofoil.xcodeproj
```

Select the `AerofoilX` target in Xcode and build.

