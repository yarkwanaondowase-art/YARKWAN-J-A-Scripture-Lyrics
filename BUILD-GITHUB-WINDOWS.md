# Build without Visual Studio on the Acer

This project is configured to use the official OBS plugin-template build system on a GitHub-hosted Windows runner. The Acer only needs a browser; Visual Studio, CMake and the OBS development SDK are not installed locally.

## One-time setup

1. Create/sign in to a GitHub account.
2. Create a new repository, for example `yarkwan-j-a-scripture-lyrics`.
3. Upload **all files and folders in this ZIP** to the repository root. Do not upload the ZIP itself as the only file.
4. Open the repository's **Actions** tab.
5. Select **Build YARKWAN J. A. Scripture & Lyrics for OBS 32.2.2**.
6. Click **Run workflow**.
7. Wait for the green check mark.
8. Open the completed workflow run and scroll to **Artifacts**.
9. Download `yarkwan-j-a-scripture-lyrics-windows-x64`.
10. Extract the downloaded ZIP. It contains the OBS plugin folder/layout.

## Installing into normal OBS on Windows

Close OBS first.

For a normal non-portable OBS installation, the plugin should be placed under:

`%PROGRAMDATA%\obs-studio\plugins\`

The final DLL should be under a structure similar to:

`%PROGRAMDATA%\obs-studio\plugins\yarkwan-j-a-scripture-lyrics\bin\64bit\yarkwan-j-a-scripture-lyrics.dll`

Then restart OBS.

If your OBS installation is portable, use the portable OBS folder's plugin layout instead. Do not mix the two layouts.

## Important

The build is pinned to OBS Studio 32.2.2 and its matching dependency/Qt bundles. This is deliberate because your installed OBS is 32.2.2.
