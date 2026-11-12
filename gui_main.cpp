#include <windows.h>
#include <shellapi.h>
#include <stdexcept>
#include <utility>
#include <vector>
#include "mainwindow.hpp"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int show) {
  SetProcessDPIAware();
  try {
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) throw std::runtime_error("Cannot read command-line arguments.");
    std::vector<XLSheet> sheets;
    try {
      for (int i = 1; i < argc; ++i) {
        std::filesystem::path path(argv[i]);
        const auto extension = path.extension().wstring();
        if (_wcsicmp(extension.c_str(), L".xlsx") == 0) sheets.emplace_back(std::move(path));
      }
    } catch (...) {
      LocalFree(argv);
      throw;
    }
    LocalFree(argv);
    MainWindow window(std::move(sheets));
    ShowWindow(window.handle(), show);
    MSG message{};
    int result;
    while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
      if (!IsDialogMessageW(window.handle(), &message)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
      }
    }
    return result == -1 ? 1 : static_cast<int>(message.wParam);
  } catch (const std::exception&) {
    MessageBoxW(nullptr, L"Cannot start CF-Reg.", L"CF-Reg", MB_OK | MB_ICONERROR);
    return 1;
  }
}
