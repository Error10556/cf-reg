#!/usr/bin/env python3
"""Bundle the native Windows executables and their MinGW runtime DLLs."""
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

root = Path(__file__).resolve().parent.parent
work = root / "build-win"
dist = work / "native-dist"
dist.mkdir(exist_ok=True)
for name in ("cfreg.exe", "cfreg-gui.exe"):
    shutil.copy2(work / "native" / name, dist / name)

licenses = dist / "licenses"
licenses.mkdir(exist_ok=True)
for dependency in (work / "native/_deps").glob("*-src"):
    for pattern in ("LICENSE*", "COPYING*"):
        for source in dependency.glob(pattern):
            if source.is_file():
                destination = licenses / dependency.name
                destination.mkdir(exist_ok=True)
                shutil.copy2(source, destination / source.name)
(dist / "README.txt").write_text(
    "CF-Reg for 64-bit Windows\n\n"
    "Launch the native WinAPI GUI with workbook paths, for example:\n"
    '  cfreg-gui.exe "participants.xlsx"\n\n'
    "cfreg.exe provides the command-line interface.\n",
    encoding="utf-8",
)
archive = work / "cfreg-windows-x86_64.zip"
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
    for source in sorted(dist.rglob("*")):
        if source.is_file():
            bundle.write(source, Path("cfreg") / source.relative_to(dist))
print(f"Windows package: {archive}")
