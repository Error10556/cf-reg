#pragma once
#include <windows.h>
#include <vector>
#include "xlmodel.hpp"

class MainWindow {
public:
  enum Control { Selection = 100, Check, Complete, Format, Status };
  explicit MainWindow(std::vector<XLSheet> sheets);
  ~MainWindow();
  MainWindow(const MainWindow&) = delete;
  MainWindow& operator=(const MainWindow&) = delete;
  HWND handle() const { return window; }
private:
  static LRESULT CALLBACK procedure(HWND, UINT, WPARAM, LPARAM);
  void create_controls();
  void layout(int width, int height);
  void perform(int action);
  std::vector<XLSheet> sheets;
  HWND window = nullptr, selection = nullptr, status = nullptr;
  HWND buttons[3]{};
};
