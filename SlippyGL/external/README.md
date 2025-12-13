# External Dependencies (Visual Studio)

This directory contains vendored header-only libraries for **Visual Studio builds on Windows**.

## Contents

- **glm/** - OpenGL Mathematics (GLM) library
  - Source: https://github.com/g-truc/glm
  - Purpose: Vector and matrix math for OpenGL
  
- **stb/stb_image.h** - STB Image library
  - Source: https://github.com/nothings/stb
  - Purpose: PNG image decoding

## Why Vendored?

For Visual Studio builds, we use fixed versions of these libraries to ensure:
- **Build stability** - No version conflicts or breaking changes
- **Quick setup** - No need to run git submodule commands on Windows
- **Consistent builds** - All developers use the same version

## Updating

To update these libraries to newer versions:

1. Navigate to the root `external/` directory (Git submodules)
2. Update the submodules:
   ```bash
   cd ../../external
   git submodule update --remote
   ```

3. Copy the updated files:
   ```powershell
   # Update stb_image.h
   Copy-Item "..\..\external\stb\stb_image.h" -Destination "stb\stb_image.h"
   
   # Update glm headers
   robocopy "..\..\external\glm\glm" "glm\glm" /E
   ```

4. Test the build and commit if everything works

## CMake Builds

For CMake builds (Linux, macOS, or cross-platform), the root `external/` directory is used instead.
See the root README.md for CMake build instructions.
