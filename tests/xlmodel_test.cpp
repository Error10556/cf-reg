#include "xlmodel.hpp"

#include <OpenXLSX.hpp>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition) {
  if (!condition) throw std::runtime_error("Workbook regression check failed");
}

void headers(OpenXLSX::XLWorksheet ws) {
  ws.cell("A1").value() = "Доступ";
  ws.cell("B1").value() = "Логин";
  ws.cell("C1").value() = "Пароль";
  ws.cell("D1").value() = "ФИО";
}

void run(const std::filesystem::path& path) {
  using namespace OpenXLSX;
  XLDocument doc;
  doc.create(path.string(), XLDoNotOverwrite);
  auto wb = doc.workbook();
  auto first = wb.worksheet(1);
  headers(first);
  first.cell("D2").value() = "First sheet";
  wb.addWorksheet("Participants");
  auto ws = wb.worksheet("Participants");
  headers(ws);
  require(ws.setActive());
  ws.cell("A2").value() = 123;
  ws.cell("B2").value() = "team009";
  ws.cell("C2").value() = "keep";
  ws.cell("D2").value() = "Алиса";
  ws.cell("D4").value() = "Боб";
  ws.cell("B4").value() = "";
  ws.cell("C4").value() = "";
  doc.save();
  doc.close();

  ::XLSheet sheet(path);
  auto info = sheet.Check();
  require(info && info->participant_count == 2 && !info->complete);
  require(sheet.Complete().has_value());
  require(sheet.Check()->complete);
  auto output = sheet.FormatForCodeforcesUTF8();
  require(output && output->starts_with("123 | team009 | keep | Алиса\n123 | team10 | "));
  require(output->ends_with(" | Боб"));
  require(sheet.Complete().has_value());
  require(sheet.FormatForCodeforcesUTF8() == output);

  doc.open(path.string());
  ws = doc.workbook().worksheet("Participants");
  require(ws.cell("C4").value().getString().size() == 10);
  require(ws.cell("D3").value().type() == XLValueType::Empty);
  require(doc.workbook().worksheet(1).cell("B2").value().type() == XLValueType::Empty);
  ws.cell("A2").value().clear();
  ws.cell("A4").value().clear();
  ws.cell("B4").value().clear();
  ws.cell("C4").value().clear();
  doc.save();
  doc.close();
  require(!sheet.Complete()); // Credentials must still be saved when contests are unavailable.
  doc.open(path.string());
  ws = doc.workbook().worksheet("Participants");
  require(ws.cell("B4").value().getString() == "team10");
  require(ws.cell("C4").value().getString().size() == 10);
  ws.cell("B2").value() = "team18446744073709551615";
  ws.cell("B4").value().clear();
  doc.save();
  doc.close();
  require(!sheet.Complete());

  doc.open(path.string());
  doc.workbook().worksheet("Participants").cell("D1").value().clear();
  doc.save();
  doc.close();
  require(sheet.Check()->participant_count == 1); // Fall back to the other matching worksheet.
  doc.open(path.string());
  doc.workbook().worksheet(1).cell("A1").value().clear();
  doc.save();
  doc.close();
  require(!sheet.Check());
  require(!sheet.Complete());
  require(!sheet.FormatForCodeforcesUTF8());
}
} // namespace

int main() {
  const auto path = std::filesystem::temp_directory_path() /
      ("cfreg-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xlsx");
  try {
    require(!XLSheet(path).Check());
    run(path);
    std::filesystem::remove(path);
  } catch (const std::exception& exc) {
    std::cerr << exc.what() << '\n';
    std::filesystem::remove(path);
    return 1;
  }
}
