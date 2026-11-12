#pragma once

#include <QWidget>
#include <vector>

#include "xlmodel.hpp"

class QComboBox;
class QLabel;

class MainWindow : public QWidget {
public:
  explicit MainWindow(std::vector<XLSheet> sheets, QWidget* parent = nullptr);

private:
  enum class Action { Check, Complete, Format };
  void perform(Action action);

  std::vector<XLSheet> sheets;
  QComboBox* selection;
  QLabel* status;
};
