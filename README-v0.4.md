# YARKWAN J. A. Scripture & Lyrics v0.4

This package keeps the lightweight native OBS design from v0.3 and adds a content-builder for the complete offline libraries.

## Libraries

- King James Version: 66 books, 1,189 chapters, 31,102 verses.
- Sacred Songs and Solos: 1,200 hymns.
- The plugin stores both as local JSON and does not require internet during worship.

## Build the libraries once

Run `python tools/build-libraries.py` on a Windows machine with internet access. The script validates the KJV counts and downloads all 1,200 SS&S hymn pages.

The resulting files are placed in `data/libraries/`.

## Important

The current chat build environment can inspect public web sources but cannot transfer remote binary archives directly into the generated plugin filesystem. Therefore this release includes the deterministic one-time downloader/normalizer instead of pretending the complete copyrighted/data archives were already embedded.

Sources used by the setup script:
- KJV: aruljohn/Bible-kjv GitHub corpus.
- SS&S: Hymnary Apps Sacred Songs and Solos numbered pages.

## v0.5 development note — Content Library

The OBS dock now includes a Content Library manager. Public-domain/free-redistribution Bible versions can be installed directly into the user's writable application-data directory. Manual JSON/TXT/CSV import remains available. Sacred Songs & Solos is exposed as a 1,200-hymn online collection installer; verify redistribution rights before repackaging the resulting lyrics file.
