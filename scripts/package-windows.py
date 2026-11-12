#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
"""Bundle the native Windows GUI and project/dependency licenses."""
from pathlib import Path
import shutil
import zipfile

root = Path(__file__).resolve().parent.parent
work = root / "build-win"
dist = work / "native-dist"
dist.mkdir(exist_ok=True)
for name in ("cfreg.exe", "README", "README.txt", "README.md"):
    (dist / name).unlink(missing_ok=True)
shutil.copy2(work / "native/cfreg-gui.exe", dist / "cfreg-gui.exe")
shutil.copy2(root / "LICENSE", dist / "LICENSE")

licenses = dist / "licenses"
licenses.mkdir(exist_ok=True)
for dependency in (work / "native/_deps").glob("*-src"):
    for pattern in ("LICENSE*", "COPYING*"):
        for source in dependency.glob(pattern):
            if source.is_file():
                destination = licenses / dependency.name
                destination.mkdir(exist_ok=True)
                shutil.copy2(source, destination / source.name)
archive = work / "cfreg-windows-x86_64.zip"
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
    for source in sorted(dist.rglob("*")):
        if source.is_file():
            bundle.write(source, Path("cfreg") / source.relative_to(dist))
print(f"Windows package: {archive}")
