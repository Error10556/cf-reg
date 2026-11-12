// SPDX-License-Identifier: 0BSD
#include "xlmodel.hpp"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <cstdint>
#include <limits>
#include <memory>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <OpenXLSX.hpp>

#include "passwords.hpp"
#include "u8.hpp"

namespace {

std::string path_utf8(const std::filesystem::path& path) {
  const auto text = path.u8string();
  return std::string(text.begin(), text.end());
}
struct recognized_column_names {
private:
  static constexpr bool any_contains(std::span<const std::u32string_view> smalls,
                                     std::u32string_view big) noexcept {
    return std::ranges::any_of(smalls, [&](auto small) { return big.contains(small); });
  }

public:
  static constexpr std::u32string_view name[] = {U"ИМЯ", U"ФАМИЛИЯ", U"ОТЧЕСТВО", U"ФИО", U"NAME"};
  static constexpr std::u32string_view contests[] = {U"КОНТЕСТ", U"ДОСТУП", U"CONTESTS"};
  static constexpr std::u32string_view login[] = {U"ЛОГИН", U"LOGIN", U"USER"};
  static constexpr std::u32string_view password[] = {U"ПАРОЛЬ", U"ПАРОЛИ", U"PASSWORD"};
  static constexpr bool is_name(std::u32string_view sv) noexcept { return any_contains(name, sv); }
  static constexpr bool is_contests(std::u32string_view sv) noexcept {
    return any_contains(contests, sv);
  }
  static constexpr bool is_login(std::u32string_view sv) noexcept {
    return any_contains(login, sv);
  }
  static constexpr bool is_password(std::u32string_view sv) noexcept {
    return any_contains(password, sv);
  }
  recognized_column_names() = delete;
};
}  // namespace

struct ColumnsOfInterest {
  uint32_t contests;
  uint32_t login;
  uint32_t password;
  uint32_t name;
};

struct SubsheetOfInterest {
  OpenXLSX::XLWorksheet worksheet;
  ColumnsOfInterest cols;
  std::vector<uint32_t> rows;
};

namespace {

std::string cell_text(OpenXLSX::XLCell cell) {
  if (cell.value().type() == OpenXLSX::XLValueType::Empty) return {};
  return cell.value().getString();
}

bool empty(OpenXLSX::XLCell cell) { return cell_text(cell).empty(); }

std::optional<SubsheetOfInterest> check_sheet(OpenXLSX::XLWorksheet ws) {
  std::optional<uint32_t> contests, login, password, name;
  const auto max_col = ws.columnCount();
  const auto max_row = ws.rowCount();
  for (uint32_t col = 1; col <= max_col; ++col) {
    auto raw = u8_to_u32(cell_text(ws.cell(1, col)));
    auto trimmed = trim(std::u32string_view(raw));
    std::u32string content;
    std::ranges::copy(trimmed | view_toupper<char32_t>, std::back_inserter(content));
    if (recognized_column_names::is_name(content)) name = col;
    else if (recognized_column_names::is_login(content)) login = col;
    else if (recognized_column_names::is_password(content)) password = col;
    else if (recognized_column_names::is_contests(content)) contests = col;
  }
  if (!contests || !login || !password || !name) return {};
  std::vector<uint32_t> rows;
  for (uint32_t row = 2; row <= max_row; ++row)
    if (!empty(ws.cell(row, *name))) rows.push_back(row);
  return SubsheetOfInterest{ws, {*contests, *login, *password, *name}, std::move(rows)};
}

// Keep the document at a stable address: OpenXLSX objects refer back to their owner.
std::expected<std::unique_ptr<OpenXLSX::XLDocument>, std::string> open_wb(const std::filesystem::path& path) {
  try {
    auto document = std::make_unique<OpenXLSX::XLDocument>();
    document->open(path_utf8(path));
    return document;
  } catch (const std::exception& exc) {
    return std::unexpected("Не открылся файл " + path_utf8(path) + ": " + exc.what());
  }
}

std::optional<SubsheetOfInterest> check_wb(OpenXLSX::XLDocument& document) {
  auto wb = document.workbook();
  const auto names = wb.worksheetNames();
  for (const auto& name : names) {
    auto ws = wb.worksheet(name);
    if (ws.isActive()) {
      if (auto result = check_sheet(ws)) return result;
      break;
    }
  }
  for (const auto& name : names) {
    auto ws = wb.worksheet(name);
    if (!ws.isActive())
      if (auto result = check_sheet(ws)) return result;
  }
  return {};
}

bool check_completeness(SubsheetOfInterest sheet) {
  for (auto row : sheet.rows)
    for (auto col : {sheet.cols.contests, sheet.cols.login, sheet.cols.password})
      if (empty(sheet.worksheet.cell(row, col))) return false;
  return true;
}

std::string error_message_no_worksheets_matched(std::string_view filename) {
  std::ostringstream out;
  std::ranges::copy(u8"Ни одна страница не содержит нужных столбцов: " | view_static_cast<char>,
                    std::ostream_iterator<char>(out));
  out << filename;
  return std::move(out).str();
}

std::tuple<std::vector<OpenXLSX::XLCell>, std::string, unsigned long long> extract_logins_to_fill(
    SubsheetOfInterest sheet) {
  auto ws = sheet.worksheet;
  std::string login_prefix = "user";
  unsigned long long login_highest = 0;
  std::vector<OpenXLSX::XLCell> logins_to_fill;
  for (auto row : sheet.rows) {
    const auto login_cell = ws.cell(row, sheet.cols.login);
    if (empty(login_cell)) {
      logins_to_fill.push_back(login_cell);
      continue;
    }
    std::string login = cell_text(login_cell);
    auto pos = login.find_last_not_of("0123456789");
    if (pos == std::string::npos || pos + 1 == login.size()) continue;
    unsigned long long number;
    auto parsed = std::from_chars(login.data() + pos + 1, login.data() + login.size(), number);
    if (parsed.ec != std::errc{} || parsed.ptr != login.data() + login.size()) continue;
    if (number >= login_highest) {
      login_highest = number;
      login_prefix = login.substr(0, pos + 1);
    }
  }
  return {std::move(logins_to_fill), login_prefix, login_highest};
}

std::vector<OpenXLSX::XLCell> extract_passwords_to_fill(SubsheetOfInterest sheet) {
  return sheet.rows |
         std::views::transform([ws = sheet.worksheet, col = sheet.cols.password](
                                   uint32_t ri) mutable { return ws.cell(ri, col); }) |
         std::views::cache_latest | std::views::filter([](OpenXLSX::XLCell cell) {
           return empty(cell);
         }) |
         std::ranges::to<std::vector<OpenXLSX::XLCell>>();
}

std::pair<std::vector<OpenXLSX::XLCell>, std::optional<std::string>> extract_contests_to_fill(
    SubsheetOfInterest sheet) {
  std::pair<std::vector<OpenXLSX::XLCell>, std::optional<std::string>> res;
  auto& [to_fill, content] = res;
  auto ws = sheet.worksheet;
  for (auto row : sheet.rows) {
    const auto cell = ws.cell(row, sheet.cols.contests);
    if (empty(cell)) {
      to_fill.push_back(cell);
      continue;
    }
    auto s = cell_text(cell);
    if (!content) content.emplace(std::move(s));
  }
  return res;
}

}  // namespace

XLSheet::XLSheet(std::filesystem::path path) : filename{path} {}

std::expected<XLSheet::Info, std::string> XLSheet::Check() const {
  using ret_t = std::expected<XLSheet::Info, std::string>;
  return open_wb(filename).and_then([this](std::unique_ptr<OpenXLSX::XLDocument> wb) -> ret_t {
    auto sheet = check_wb(*wb);
    if (!sheet) return std::unexpected(error_message_no_worksheets_matched(path_utf8(filename)));
    return {{sheet->rows.size(), check_completeness(*sheet)}};
  });
}

std::expected<void, std::string> XLSheet::Complete() const {
  auto wb = open_wb(filename);
  if (!wb) return std::unexpected(wb.error());
  auto sheet = check_wb(**wb);
  if (!sheet) return std::unexpected(error_message_no_worksheets_matched(path_utf8(filename)));

  auto [logins, prefix, highest] = extract_logins_to_fill(*sheet);
  auto passwords = extract_passwords_to_fill(*sheet);
  auto [contests, contest_content] = extract_contests_to_fill(*sheet);

  if (logins.size() > std::numeric_limits<unsigned long long>::max() - highest)
    return std::unexpected("Login numbering would overflow.");

  for (auto cell : logins) cell.value() = prefix + std::to_string(++highest);
  PasswordGenerator generate_password;
  for (auto cell : passwords) cell.value() = generate_password(10);
  if (contest_content)
    for (auto cell : contests) cell.value() = *contest_content;

  try {
    (*wb)->save();
  } catch (const std::exception& exc) {
    return std::unexpected("Не удалось сохранить документ " + path_utf8(filename) + ": " + exc.what());
  }
  if (!contest_content)
    return std::unexpected(
        u8"Столбец контестов не заполнен. "
        u8"Вручную введите значение в одну ячейку и попробуйте снова." |
        view_static_cast<char> | std::ranges::to<std::string>());
  return {};
}

std::expected<std::string, std::string> XLSheet::FormatForCodeforcesUTF8() const {
  auto wb = open_wb(filename);
  if (!wb) return std::unexpected(wb.error());
  auto sheet = check_wb(**wb);
  if (!sheet) return std::unexpected(error_message_no_worksheets_matched(path_utf8(filename)));
  std::ostringstream result;
  bool first_row = true;
  for (auto row : sheet->rows) {
    if (!first_row) result << '\n';
    first_row = false;
    bool first_col = true;
    for (auto col :
         {sheet->cols.contests, sheet->cols.login, sheet->cols.password, sheet->cols.name}) {
      if (!first_col) result << " | ";
      first_col = false;
      result << cell_text(sheet->worksheet.cell(row, col));
    }
  }
  return std::move(result).str();
}

const std::filesystem::path& XLSheet::Filename() const { return filename; }
