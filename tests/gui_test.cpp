#include "mainwindow.hpp"
#include "sheets.hpp"

#include <OpenXLSX.hpp>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition) {
  if (!condition) throw std::runtime_error("GUI regression check failed");
}

void run() {
  MainWindow empty({});
  require(!empty.findChild<QPushButton*>("checkButton")->isEnabled());
  require(!empty.findChild<QPushButton*>("completeButton")->isEnabled());
  require(!empty.findChild<QPushButton*>("formatButton")->isEnabled());

  QTemporaryDir dir;
  require(dir.isValid());
  auto path = dir.filePath("participants.XLSX").toStdString();
  auto missing = dir.filePath("missing.xlsx").toStdString();
  std::string ignored = "ignored.txt";
  OpenXLSX::XLDocument doc;
  doc.create(path, OpenXLSX::XLDoNotOverwrite);
  auto ws = doc.workbook().worksheet(1);
  ws.cell("A1").value() = "Contests";
  ws.cell("B1").value() = "Login";
  ws.cell("C1").value() = "Password";
  ws.cell("D1").value() = "Name";
  ws.cell("A2").value() = "123";
  ws.cell("D2").value() = "Алиса";
  doc.save();
  doc.close();

  char* args[] = {path.data(), ignored.data(), missing.data()};
  MainWindow window(sheets_from_paths(args));
  auto* selection = window.findChild<QComboBox*>("workbookSelection");
  auto* status = window.findChild<QLabel*>("status");
  auto* check = window.findChild<QPushButton*>("checkButton");
  auto* complete = window.findChild<QPushButton*>("completeButton");
  auto* format = window.findChild<QPushButton*>("formatButton");
  require(selection->count() == 2 && selection->itemText(0) == QString::fromStdString(path));
  check->click();
  require(status->text().contains("Participants: 1") && status->text().contains("Complete: no"));
  complete->click();
  require(status->text() == "Workbook completed and saved.");
  check->click();
  require(status->text().contains("Complete: yes"));
  format->click();
  const auto copied = QApplication::clipboard()->text();
  require(copied.startsWith("123 | user1 | ") && copied.endsWith(QString::fromUtf8(" | Алиса")));
  require(status->text() == "Codeforces table copied to clipboard.");
  selection->setCurrentIndex(1);
  require(status->text().isEmpty());
  check->click();
  require(status->text().startsWith("Error:"));
  complete->click();
  require(status->text().startsWith("Error:"));
  format->click();
  require(status->text().startsWith("Error:"));
  require(QApplication::clipboard()->text() == copied);
}
} // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  try {
    run();
  } catch (const std::exception& exc) {
    std::cerr << exc.what() << '\n';
    return 1;
  }
}
