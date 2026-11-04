#!/usr/bin/env python3
"""Copy SimulIDE's downloadable component sets next to the web app.

Browsers refuse to download files from another site unless that site allows
it (CORS), and simulide.com does not. The web build therefore installs
component sets (Arduino, AVR, PIC, ...) from <app>/components/, which this
script fills from simulide.com at deploy time.

Usage: mirror_components.py <dist-dir>
"""
import os, sys, time, urllib.parse, urllib.request

BASE = "https://simulide.com/p/direct_downloads/components/dloadset.php?file="
UA = {"User-Agent": "SimulIDE-web-mirror (+https://github.com)"}

def fetch(name, tries=3):
    url = BASE + urllib.parse.quote(name)
    for i in range(tries):
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=60) as r:
                return r.read()
        except Exception as e:
            print(f"  retry {name}: {e}")
            time.sleep(3 * (i + 1))
    raise RuntimeError(f"could not download {name}")

def main():
    out = os.path.join(sys.argv[1], "components")
    os.makedirs(out, exist_ok=True)
    listing = fetch("components.txt")
    with open(os.path.join(out, "components.txt"), "wb") as f:
        f.write(listing)

    failed = 0
    for line in listing.decode("utf-8", "replace").splitlines():
        fields = line.split("; ")
        if len(fields) < 4:          # category header line
            continue
        name = fields[2].strip()
        if not name or ".." in name or name.startswith("/"):
            continue
        dest = os.path.join(out, name)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        try:
            data = fetch(name)
            with open(dest, "wb") as f:
                f.write(data)
            print(f"ok   {name} ({len(data)//1024} KB)")
        except Exception as e:
            failed += 1
            print(f"FAIL {name}: {e}")
    print(f"done, {failed} failed")

if __name__ == "__main__":
    main()
