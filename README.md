# YARKWAN J. A. Scripture & Lyrics — v0.5

Native OBS Studio plugin for lightweight Bible and worship-song presentation, with downloadable content libraries and manual import.

## v0.5 workflow

1. Open the YARKWAN J. A. Scripture & Lyrics dock in OBS.
2. Install a Bible from **Bible Library…**, or import your own `.json`, `.txt` or `.csv` file. `.bib` remains reserved for the future Tiv/BibleShow importer.
3. Select Book → Chapter → Verse or search verse text.
4. Install a hymn collection from **Hymn Library…**, or import your own `.json`, `.txt` or `.csv` songs. TXT songs use `# Song Title` and `[Verse]`, `[Chorus]`, `[Bridge]` section markers.
5. Preview the selected text in the dock.
6. Add a Scripture or Lyrics OBS source to the current scene.
7. Choose Lower Third or Full Screen and press SHOW / UPDATE.
8. Previous / Next moves through verses or song sections.

## Example files

`data/examples/kjv-demo.json` is a small public-domain KJV demonstration library, not a complete Bible.

`data/examples/worship-songs.txt` demonstrates the song import format. Replace the demo song text with lyrics you are authorized to use.

## Planned

- persistent library database
- richer song editing/import
- full Bible search/results list
- automatic verse/section progression
- keyboard shortcuts
- configurable colors, transparency, font, alignment and shadow
- dedicated Tiv Bible file format/importer
- optional BibleShow `.BIB` importer if its format can be supported without bypassing protection or licensing restrictions

## Build

Requires an OBS Studio development environment with libobs, obs-frontend-api and Qt 6 development packages. This source package is not itself a ready-to-install Windows DLL.
