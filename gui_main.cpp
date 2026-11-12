#include <QApplication>
#include <span>
#include <utility>

#include "mainwindow.hpp"
#include "sheets.hpp"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  auto sheets = sheets_from_paths(std::span<char*>{argv + 1, static_cast<size_t>(argc - 1)});
  MainWindow window(std::move(sheets));
  window.show();
  return app.exec();
}
