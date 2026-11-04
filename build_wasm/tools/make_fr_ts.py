#!/usr/bin/env python3
"""Fill resources/translations/simulide_fr.ts from fr_strings.py.
Refresh the .ts first with:  lupdate -recursive src -extensions cpp,h,ui -ts resources/translations/simulide_fr.ts
"""
import os, sys, xml.etree.ElementTree as ET
sys.path.insert(0, os.path.dirname(__file__))
from fr_strings import FR
path = sys.argv[1] if len(sys.argv) > 1 else "resources/translations/simulide_fr.ts"
tree = ET.parse(path); root = tree.getroot(); root.set("language", "fr_FR")
missing = set()
for m in root.iter("message"):
    src = m.find("source").text or ""
    tr = m.find("translation")
    if src in FR:
        if m.get("numerus") == "yes":
            for n in list(tr): tr.remove(n)
            for _ in range(2):
                ET.SubElement(tr, "numerusform").text = FR[src].replace("%n", "%n")
        else:
            tr.text = FR[src]
        tr.attrib.pop("type", None)
    else:
        missing.add(src)
tree.write(path, encoding="utf-8", xml_declaration=True)
print("missing:", sorted(missing))
