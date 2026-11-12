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

search = [Path("/usr/x86_64-w64-mingw32/bin")]
for name in ("libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll"):
    location = subprocess.check_output(
        ["x86_64-w64-mingw32-g++", f"-print-file-name={name}"], text=True
    ).strip()
    if Path(location).is_file():
        search.append(Path(location).resolve().parent)
available = {p.name.lower(): p for folder in search for p in folder.glob("*.dll")}
pending = list(dist.glob("*.exe"))
seen = set()
while pending:
    binary = pending.pop()
    if binary.name.lower() in seen:
        continue
    seen.add(binary.name.lower())
    imports = subprocess.check_output(["x86_64-w64-mingw32-objdump", "-p", binary], text=True)
    for name in re.findall(r"DLL Name:\s*(\S+)", imports):
        source = available.get(name.lower())
        if source is not None and name.lower() not in seen:
            target = dist / source.name
            shutil.copy2(source, target)
            pending.append(target)
        elif name.lower().startswith(("libgcc", "libstdc++", "libwinpthread")) and source is None:
            raise FileNotFoundError(f"Cannot locate MinGW runtime {name}")

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
    "Keep the DLLs beside the executables.\n"
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
