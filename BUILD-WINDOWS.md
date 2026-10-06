# Building the Windows OBS plugin

The easiest reproducible route is the official OBS plugin build environment. OBS publishes an official plugin template with CMake and GitHub Actions support; its current Windows template documents Visual Studio 17 2022 and CMake 3.30.5 as supported build tools.

## Recommended route

1. Put this project in a GitHub repository.
2. Push the project.
3. Open **Actions → Build Windows OBS plugin**.
4. Run **workflow_dispatch**.
5. Download the generated `yarkwan-j-a-scripture-lyrics-windows-x64` artifact.

This avoids installing the large OBS development toolchain on the Acer laptop.

## Local build

A local Windows build requires the OBS development dependencies, Qt 6, CMake and Visual Studio. The current CMake project expects `libobs`, `obs-frontend-api` and Qt 6 to be discoverable by CMake.

The plugin itself is designed to be lightweight at runtime; the development toolchain is much larger than the final plugin.
