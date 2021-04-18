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

Changes not tested to see if they break other platforms yet.

Compiles but crashes due to a surprise null pixel shader.

### Irrelevant log messages

`AddInstanceForFactory` looks like it's AVFoundation-related. These messages are printed before the program exits early, so are unlikely to be related to the problem.

```
2021-03-30 18:24:47.419584-0700 AerofoilX[5744:5185611] Metal API Validation Enabled
2021-03-30 18:24:47.447837-0700 AerofoilX[5744:5186044] flock failed to lock maps file: errno = 35
2021-03-30 18:24:47.448157-0700 AerofoilX[5744:5186044] flock failed to lock maps file: errno = 35
2021-03-30 18:24:47.509443-0700 AerofoilX[5744:5185611] [plugin] AddInstanceForFactory: No factory registered for id <CFUUID 0x100c12790> F8BB1C28-BAE8-11D6-9C31-00039315CD46
Program ended with exit code: 0
```

### Resource archive missing

Failing in `ResourceManagerImpl::GetAppResource` during `LoadCursors` in `AppStartup` because `m_appResArchive` is null.

`PL_Init` from the Portability Layer (MacOS Classic emulation functions) is supposed to initialize the `ResourceManager` earlier in `AppStartup` with an `IResourceArchive` for this file, which is stored in `ApplicationResources.gpf`.

We don't have an `ApplicationResources.gpf`. This gets built for Windows when `ReleasePackageInstaller/ReleasePackageInstaller.wixproj` (the WiX Installer packaging script) calls `ConvertResources.bat`, which creates `Packaged/ApplicationResources.gpf` and `Packaged/Houses/*.gpf` from equivalents in `GliderProData`. So rather than try to port `ConvertResources.bat` and the utilities it calls, or dig up some antique macOS-specific resource APIs, let's grab the `Packaged` directory from the Windows ZIP release of Aerofoil.

_TODO: port `ConvertResources.bat` so we can build game data from source anywhere._

`Packaged` has to be copied to the `Debug` folder, because that's where Xcode writes the `AerofoilX` binary when building with default settings.

**Fixed via workaround of using .gpf files built on Windows.**

### Convenience logger

Created `GpLogDriver_Clog`, a portable logger that writes to `clog`, the C++ equivalent of `stderr`, and temporarily switched the SDL port to use that instead of the file-backed `GpLogDriver_X`. This is more convenient than a file log when developing with Xcode.

Noticed problems with `GpLogDriver_Clog` due to `va_list` reuse in copied code. Fixed issue in other loggers.

Added `[INFO]` debug tag to all loggers.
