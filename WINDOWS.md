# Building for Windows from Linux

```sh
bash scripts/build-windows.sh
bash scripts/test-windows.sh
```

The build creates the native WinAPI GUI `cfreg-gui.exe` and CLI `cfreg.exe`,
then packages them with the required MinGW runtime DLLs in
`build-win/cfreg-windows-x86_64.zip`. Extract the archive and keep the DLLs
beside the executables. Launch with workbook paths:

```powershell
.\cfreg-gui.exe "participants.xlsx"
```

Prerequisites: MinGW-w64 GCC 15 or newer with its matching standard library,
CMake 3.30 or newer, Ninja, Git, and Python 3. Wine is needed only for tests.
Qt is not used. The toolchain assumes `x86_64-w64-mingw32-*` commands on PATH
and a target root at `/usr/x86_64-w64-mingw32`. Set `JOBS` to control parallelism.

The build directory is `build-win/native`; the unpacked package is in
`build-win/native-dist`. Rebuilding reuses the downloaded dependencies.
Tests use an isolated Wine prefix at `build-win/wine-test` and exercise workbook
operations, native controls, Cyrillic filenames, and Unicode clipboard output.
A desktop/display is required for the GUI test. Actual Windows testing is still
recommended after Wine verification.

On Linux the default build provides the CLI. The GUI requires a Windows target.
Old Qt build artifacts under `build-win/qt*` are unused and can be removed.
