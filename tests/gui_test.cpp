// SPDX-License-Identifier: 0BSD
#include "mainwindow.hpp"
#include <OpenXLSX.hpp>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <source_location>
#include <stdexcept>

namespace {
void require(bool condition, std::source_location location = std::source_location::current()) {
  if (!condition) throw std::runtime_error("GUI regression check failed at line " +
                                          std::to_string(location.line()));
}
std::wstring text(HWND control) {
  std::wstring result(GetWindowTextLengthW(control) + 1, L'\0');
  result.resize(GetWindowTextW(control, result.data(), static_cast<int>(result.size())));
  return result;
}
std::wstring clipboard(HWND owner) {
  bool opened = false;
  for (int attempt = 0; attempt < 50 && !(opened = OpenClipboard(owner)); ++attempt)
    Sleep(10); // Another desktop process may briefly own the clipboard.
  require(opened);
  auto data = GetClipboardData(CF_UNICODETEXT);
  auto value = data ? static_cast<const wchar_t*>(GlobalLock(data)) : nullptr;
  std::wstring result = value ? value : L"";
  if (value) GlobalUnlock(data);
  CloseClipboard();
  require(value != nullptr);
  return result;
}
std::string utf8(const std::filesystem::path& path) {
  auto value = path.u8string();
  return std::string(value.begin(), value.end());
}
void run(const std::filesystem::path& dir) {
  MainWindow empty({});
  for (int id : {MainWindow::Check, MainWindow::Complete, MainWindow::Format})
    require(!IsWindowEnabled(GetDlgItem(empty.handle(), id)));
  const auto path = dir / L"участники.XLSX";
  OpenXLSX::XLDocument doc;
  doc.create(utf8(path), OpenXLSX::XLDoNotOverwrite);
  auto ws = doc.workbook().worksheet(1);
  ws.cell("A1").value() = "Contests";
  ws.cell("B1").value() = "Login";
  ws.cell("C1").value() = "Password";
  ws.cell("D1").value() = "Name";
  ws.cell("A2").value() = "123";
  ws.cell("D2").value() = "Алиса";
  doc.save();
  doc.close();

  MainWindow window({XLSheet(path), XLSheet(dir / "missing.xlsx")});
  HWND hwnd = window.handle();
  ShowWindow(hwnd, SW_SHOWNORMAL);
  SetActiveWindow(hwnd);
  HWND selection = GetDlgItem(hwnd, MainWindow::Selection);
  HWND status = GetDlgItem(hwnd, MainWindow::Status);
  require(SendMessageW(selection, CB_GETCOUNT, 0, 0) == 2);
  std::wstring item(path.wstring().size() + 1, L'\0');
  SendMessageW(selection, CB_GETLBTEXT, 0, reinterpret_cast<LPARAM>(item.data()));
  item.resize(path.wstring().size());
  require(item == path.wstring());
  auto click = [hwnd](int id) {
    SendMessageW(GetDlgItem(hwnd, id), BM_CLICK, 0, 0);
  };
  require(text(status).contains(L"Participants: 1") && text(status).contains(L"Complete: no"));
  click(MainWindow::Complete);
  require(text(status).contains(L"Participants: 1") && text(status).contains(L"Complete: yes"));
  click(MainWindow::Format);
  const auto copied = clipboard(hwnd);
  require(copied.starts_with(L"123 | user1 | ") && copied.ends_with(L" | Алиса"));
  require(text(status) == L"Codeforces table copied to clipboard.");
  SendMessageW(selection, CB_SETCURSEL, 1, 0);
  SendMessageW(hwnd, WM_COMMAND, MAKEWPARAM(MainWindow::Selection, CBN_SELCHANGE),
               reinterpret_cast<LPARAM>(selection));
  require(text(status).starts_with(L"Error:"));
  for (int id : {MainWindow::Check, MainWindow::Complete, MainWindow::Format}) {
    click(id);
    require(text(status).starts_with(L"Error:"));
  }
  require(clipboard(hwnd) == copied);

  SendMessageW(selection, CB_SETCURSEL, 0, 0);
  SendMessageW(hwnd, WM_COMMAND, MAKEWPARAM(MainWindow::Selection, CBN_SELCHANGE),
               reinterpret_cast<LPARAM>(selection));
  require(text(status).contains(L"Participants: 1") && text(status).contains(L"Complete: yes"));

  // Complete may save partial changes and still report missing contest data.
  doc.open(utf8(path));
  doc.workbook().worksheet(1).cell("A2").value().clear();
  doc.save();
  doc.close();
  click(MainWindow::Complete);
  require(text(status).contains(L"Complete: no") && text(status).contains(L"Error:"));
}
} // namespace

int main() {
  const auto dir = std::filesystem::temp_directory_path() /
      ("cfreg-gui-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    std::filesystem::create_directory(dir);
    run(dir);
    std::filesystem::remove_all(dir);
  } catch (const std::exception& exc) {
    std::cerr << exc.what() << '\n';
    std::filesystem::remove_all(dir);
    return 1;
  }
}
