#!/usr/bin/env python3
"""Download and normalize the offline Bible + hymn libraries for YARKWAN J. A.

Sources:
  KJV: aruljohn/Bible-kjv (66 separate JSON books)
  SS&S: Hymnary Apps, /sacred-songs/no/<number>, 1..1200

The resulting files are intentionally plain JSON so the OBS plugin can remain
fully offline after this one-time content build.
"""
from __future__ import annotations
import concurrent.futures
import html
import json
import re
import shutil
import sys
import tempfile
import time
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "data" / "libraries"
OUT.mkdir(parents=True, exist_ok=True)
KJV_ZIP = "https://github.com/aruljohn/Bible-kjv/archive/refs/heads/master.zip"
HYMN_URL = "https://www.hymnaryapps.com/sacred-songs/no/{n}"
UA = "YARKWAN-J-A-Scripture-Lyrics/0.4"

BOOK_ORDER = [
    "Genesis","Exodus","Leviticus","Numbers","Deuteronomy","Joshua","Judges","Ruth",
    "1 Samuel","2 Samuel","1 Kings","2 Kings","1 Chronicles","2 Chronicles","Ezra","Nehemiah","Esther",
    "Job","Psalms","Proverbs","Ecclesiastes","Song of Solomon","Isaiah","Jeremiah","Lamentations","Ezekiel","Daniel",
    "Hosea","Joel","Amos","Obadiah","Jonah","Micah","Nahum","Habakkuk","Zephaniah","Haggai","Zechariah","Malachi",
    "Matthew","Mark","Luke","John","Acts","Romans","1 Corinthians","2 Corinthians","Galatians","Ephesians","Philippians","Colossians",
    "1 Thessalonians","2 Thessalonians","1 Timothy","2 Timothy","Titus","Philemon","Hebrews","James","1 Peter","2 Peter","1 John","2 John","3 John","Jude","Revelation"
]

def get(url: str, timeout=60) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read()

def build_kjv():
    print("Downloading KJV source…")
    with tempfile.TemporaryDirectory() as td:
        zpath = Path(td) / "kjv.zip"
        zpath.write_bytes(get(KJV_ZIP, 120))
        with zipfile.ZipFile(zpath) as z:
            names = {Path(n).name: n for n in z.namelist() if n.lower().endswith('.json')}
            books = []
            for name in BOOK_ORDER:
                fn = name + ".json"
                member = names.get(fn)
                if not member:
                    raise RuntimeError(f"Missing KJV book: {fn}")
                obj = json.loads(z.read(member).decode('utf-8'))
                book_name = obj.get('book') or name
                chapters = []
                for c in obj.get('chapters', []):
                    ch = int(c.get('chapter'))
                    verses = []
                    for v in c.get('verses', []):
                        verses.append({"verse": int(v.get('verse')), "text": v.get('text','').strip()})
                    chapters.append({"chapter": ch, "verses": verses})
                books.append({"name": book_name, "chapters": chapters})
    chapters = sum(len(b['chapters']) for b in books)
    verses = sum(len(c['verses']) for b in books for c in b['chapters'])
    if len(books) != 66 or chapters != 1189 or verses != 31102:
        raise RuntimeError(f"KJV validation failed: {len(books)} books, {chapters} chapters, {verses} verses")
    out = {"format":"yarkwan-bible-v1","title":"King James Version","language":"en","copyright":"Public domain","books":books}
    (OUT / "kjv.json").write_text(json.dumps(out, ensure_ascii=False, separators=(',',':')), encoding='utf-8')
    print(f"KJV ready: {len(books)} books, {chapters} chapters, {verses} verses")

def strip_tags(fragment: str) -> str:
    fragment = re.sub(r'<br\s*/?>', '\n', fragment, flags=re.I)
    fragment = re.sub(r'</(?:p|div|li|h[1-6]|tr)>', '\n', fragment, flags=re.I)
    fragment = re.sub(r'<[^>]+>', '', fragment)
    return html.unescape(fragment).replace('\r','')

def parse_hymn(n: int, raw: bytes):
    s = raw.decode('utf-8', errors='replace')
    title_m = re.search(r'<h[1-3][^>]*>\s*([^<]*?)\s*</h[1-3]>', s, re.I)
    title = html.unescape(title_m.group(1)).strip() if title_m else f"Hymn {n}"
    first = re.search(r'First Line:\s*(.*?)</', s, re.I|re.S)
    first_line = strip_tags(first.group(1)).strip() if first else ''
    # Capture the main content after First Line and before common footer/navigation.
    start = first.end() if first else 0
    tail = s[start:]
    for marker in ('<footer', 'Download Sacred Songs', 'Download the Sacred Songs'):
        pos = tail.lower().find(marker.lower())
        if pos >= 0:
            tail = tail[:pos]
    text = strip_tags(tail)
    lines = []
    for line in text.splitlines():
        line = re.sub(r'\s+', ' ', line).strip()
        if not line or line.lower().startswith(('hymn no.', 'download')): continue
        lines.append(line)
    # The first line can be duplicated in page content; retain the actual lyrics.
    lyrics = '\n'.join(lines).strip()
    if first_line and first_line not in lyrics:
        lyrics = (first_line + '\n' + lyrics).strip()
    if not lyrics:
        raise RuntimeError(f"No lyrics extracted for hymn {n}")
    return {"number": n, "title": title, "language":"en", "sections":[{"label":"Lyrics","text":lyrics}]}

def build_hymns():
    print("Downloading SS&S 1,200 hymn pages…")
    def one(n):
        last = None
        for attempt in range(3):
            try:
                return parse_hymn(n, get(HYMN_URL.format(n=n), 45))
            except Exception as e:
                last = e; time.sleep(0.5 * (attempt+1))
        raise last
    songs=[]
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as ex:
        futs={ex.submit(one,n):n for n in range(1,1201)}
        for i,f in enumerate(concurrent.futures.as_completed(futs),1):
            n=futs[f]
            songs.append(f.result())
            if i % 50 == 0: print(f"  {i}/1200")
    songs.sort(key=lambda x:x['number'])
    out={"title":"Sacred Songs and Solos — 1200 Hymns","songs":songs}
    (OUT / "sacred-songs-and-solos-1200.json").write_text(json.dumps(out, ensure_ascii=False, separators=(',',':')), encoding='utf-8')
    print("SS&S ready: 1200 hymns")

if __name__ == '__main__':
    build_kjv()
    build_hymns()
    print("All libraries built in", OUT)
