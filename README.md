# starlight
[![Build status](https://ci.appveyor.com/api/projects/status/nwu5skix98cb35ua?svg=true)](https://ci.appveyor.com/project/darkedge/starlight)

## How to build (Windows)
- Run install.ps1 (This downloads and extracts the dependencies, may take a while)
- Run cook.ps1 (Copies assets to output folder)
- Open starlight.sln

Now you have two options:
- Set the build configuration to Debug/Release and build the starlight_win32 project
- Set the build configuration to UB Debug/UB Release and build the starlight_win32_ub project

The UB version is a unity build, which compiles about twice as fast on my machines.
