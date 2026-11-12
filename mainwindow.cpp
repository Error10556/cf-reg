#include "mainwindow.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace {
std::wstring from_utf8(const std::string& text) {
  if (text.empty()) return {};
  const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                      static_cast<int>(text.size()), nullptr, 0);
  if (!size) throw std::runtime_error("Invalid UTF-8 text.");
  std::wstring result(size, L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                      static_cast<int>(text.size()), result.data(), size);
  return result;
}

void copy_text(HWND owner, const std::wstring& text) {
  const auto bytes = (text.size() + 1) * sizeof(wchar_t);
  HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (!memory) throw std::runtime_error("Cannot allocate clipboard memory.");
  void* destination = GlobalLock(memory);
  if (!destination) {
    GlobalFree(memory);
    throw std::runtime_error("Cannot lock clipboard memory.");
  }
  std::memcpy(destination, text.c_str(), bytes);
  GlobalUnlock(memory);
  if (!OpenClipboard(owner)) {
    GlobalFree(memory);
    throw std::runtime_error("Cannot open the clipboard. Try again.");
  }
  const bool copied = EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, memory);
  CloseClipboard();
  if (!copied) {
    GlobalFree(memory);
    throw std::runtime_error("Cannot copy to the clipboard.");
  }
  // Windows owns memory after SetClipboardData succeeds.
}
} // namespace

MainWindow::MainWindow(std::vector<XLSheet> sheets) : sheets(std::move(sheets)) {
  const auto instance = GetModuleHandleW(nullptr);
  WNDCLASSW cls{};
  cls.lpfnWndProc = procedure;
  cls.hInstance = instance;
  cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
  cls.lpszClassName = L"CFRegMainWindow";
  if (!RegisterClassW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    throw std::runtime_error("Cannot register the window class.");
  window = CreateWindowExW(WS_EX_CONTROLPARENT, cls.lpszClassName, L"CF-Reg",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 720, 260,
      nullptr, nullptr, instance, this);
  if (!window) throw std::runtime_error("Cannot create the main window.");
}

MainWindow::~MainWindow() {
  if (window && IsWindow(window)) DestroyWindow(window);
}

void MainWindow::create_controls() {
  auto control = [this](const wchar_t* type, const wchar_t* text, DWORD style, int id) {
    auto child = CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style,
        0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
    if (!child) throw std::runtime_error("Cannot create a window control.");
    SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    return child;
  };
  selection = control(L"COMBOBOX", L"", WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, Selection);
  const wchar_t* labels[] = {L"Check", L"Complete and save", L"Format for Codeforces"};
  for (int i = 0; i < 3; ++i) {
    buttons[i] = control(L"BUTTON", labels[i], WS_TABSTOP | BS_PUSHBUTTON, Check + i);
    EnableWindow(buttons[i], !sheets.empty());
  }
  status = control(L"EDIT", L"", WS_TABSTOP | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, Status);
  for (const auto& sheet : sheets) {
    const auto name = sheet.Filename().wstring();
    SendMessageW(selection, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
  }
  if (sheets.empty()) {
    EnableWindow(selection, FALSE);
    SetWindowTextW(status, L"No workbooks supplied. Launch cfreg-gui with one or more .xlsx file paths.");
  } else {
    SendMessageW(selection, CB_SETCURSEL, 0, 0);
  }
}

void MainWindow::layout(int width, int height) {
  const int content = std::max(0, width - 24);
  MoveWindow(selection, 12, 12, content, 260, TRUE);
  const int button_width = std::max(0, (content - 16) / 3);
  for (int i = 0; i < 3; ++i)
    MoveWindow(buttons[i], 12 + i * (button_width + 8), 48, button_width, 32, TRUE);
  MoveWindow(status, 12, 92, content, std::max(0, height - 104), TRUE);
}

void MainWindow::perform(int action) {
  const auto index = SendMessageW(selection, CB_GETCURSEL, 0, 0);
  if (index == CB_ERR || static_cast<size_t>(index) >= sheets.size()) return;
  const auto& sheet = sheets[static_cast<size_t>(index)];
  auto set_status = [this](const std::wstring& text) { SetWindowTextW(status, text.c_str()); };
  try {
    switch (action) {
      case Check: {
        auto result = sheet.Check();
        if (result)
          set_status(L"Participants: " + std::to_wstring(result->participant_count) +
                     L"\r\nComplete: " + (result->complete ? L"yes" : L"no"));
        else
          set_status(L"Error: " + from_utf8(result.error()));
        break;
      }
      case Complete: {
        auto result = sheet.Complete();
        set_status(result ? L"Workbook completed and saved." : L"Error: " + from_utf8(result.error()));
        break;
      }
      case Format: {
        auto result = sheet.FormatForCodeforcesUTF8();
        if (result) {
          copy_text(window, from_utf8(*result));
          set_status(L"Codeforces table copied to clipboard.");
        } else {
          set_status(L"Error: " + from_utf8(result.error()));
        }
        break;
      }
    }
  } catch (const std::exception& exc) {
    set_status(L"Error: " + from_utf8(exc.what()));
  }
}

LRESULT CALLBACK MainWindow::procedure(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
  auto self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (message == WM_NCCREATE) {
    self = static_cast<MainWindow*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
    self->window = hwnd;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
  }
  if (!self) return DefWindowProcW(hwnd, message, wp, lp);
  // No C++ exception may cross the Windows callback boundary.
  try {
    switch (message) {
      case WM_CREATE: self->create_controls(); return 0;
      case WM_SIZE: self->layout(LOWORD(lp), HIWORD(lp)); return 0;
      case WM_GETMINMAXINFO: {
        auto info = reinterpret_cast<MINMAXINFO*>(lp);
        info->ptMinTrackSize = {540, 220};
        return 0;
      }
      case WM_COMMAND:
        if (LOWORD(wp) == Selection && HIWORD(wp) == CBN_SELCHANGE) SetWindowTextW(self->status, L"");
        else if (HIWORD(wp) == BN_CLICKED && LOWORD(wp) >= Check && LOWORD(wp) <= Format)
          self->perform(LOWORD(wp));
        return 0;
      case WM_DESTROY: PostQuitMessage(0); return 0;
      case WM_NCDESTROY:
        self->window = nullptr;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        break;
    }
  } catch (...) {
    if (message == WM_CREATE) return -1;
    SetWindowTextW(self->status, L"Error: cannot complete this operation.");
    return 0;
  }
  return DefWindowProcW(hwnd, message, wp, lp);
}
