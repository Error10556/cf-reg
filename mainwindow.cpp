#include "mainwindow.hpp"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <exception>
#include <utility>

namespace {
QString utf8(const std::string& text) {
  return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}
}  // namespace

MainWindow::MainWindow(std::vector<XLSheet> sheets, QWidget* parent)
    : QWidget(parent), sheets(std::move(sheets)), selection(new QComboBox(this)),
      status(new QLabel(this)) {
  setWindowTitle("CF-Reg");
  resize(600, 180);
  auto* layout = new QVBoxLayout(this);
  selection->setObjectName("workbookSelection");
  selection->setAccessibleName("Workbook");
  selection->setMinimumContentsLength(30);
  selection->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  for (const auto& sheet : this->sheets) {
    const auto path = sheet.Filename().u8string();
    selection->addItem(QString::fromUtf8(reinterpret_cast<const char*>(path.data()),
                                        static_cast<qsizetype>(path.size())));
  }
  layout->addWidget(selection);

  auto* buttons = new QHBoxLayout;
  auto add_button = [&](const char* text, const char* name, Action action) {
    auto* button = new QPushButton(text, this);
    button->setObjectName(name);
    button->setEnabled(!this->sheets.empty());
    buttons->addWidget(button);
    connect(button, &QPushButton::clicked, this, [this, action] { perform(action); });
  };
  add_button("Check", "checkButton", Action::Check);
  add_button("Complete", "completeButton", Action::Complete);
  add_button("Format for Codeforces", "formatButton", Action::Format);
  layout->addLayout(buttons);
  status->setObjectName("status");
  status->setTextFormat(Qt::PlainText);
  status->setWordWrap(true);
  status->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(status);
  layout->addStretch();
  connect(selection, &QComboBox::currentIndexChanged, this, [this] { status->clear(); });
  if (this->sheets.empty()) {
    selection->setEnabled(false);
    status->setText("No workbooks supplied. Launch cfreg-qt with one or more .xlsx file paths.");
  }
}

void MainWindow::perform(Action action) {
  const auto index = selection->currentIndex();
  if (index < 0 || static_cast<size_t>(index) >= sheets.size()) return;
  const auto& sheet = sheets[static_cast<size_t>(index)];
  try {
    switch (action) {
      case Action::Check: {
        auto result = sheet.Check();
        if (result)
          status->setText(QString("Participants: %1\nComplete: %2")
                              .arg(static_cast<qulonglong>(result->participant_count))
                              .arg(result->complete ? "yes" : "no"));
        else
          status->setText("Error: " + utf8(result.error()));
        break;
      }
      case Action::Complete: {
        auto result = sheet.Complete();
        status->setText(result ? "Workbook completed and saved." : "Error: " + utf8(result.error()));
        break;
      }
      case Action::Format: {
        auto result = sheet.FormatForCodeforcesUTF8();
        if (result) {
          QApplication::clipboard()->setText(utf8(*result));
          status->setText("Codeforces table copied to clipboard.");
        } else {
          status->setText("Error: " + utf8(result.error()));
        }
        break;
      }
    }
  } catch (const std::exception& exc) {
    status->setText("Error: " + QString::fromUtf8(exc.what()));
  }
}
