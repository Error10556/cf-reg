#include "xlmodel.hpp"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <generator>
#include <iterator>
#include <limits>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "passwords.hpp"
#include "u8.hpp"
#include "xlnt/worksheet/major_order.hpp"

using std::literals::operator""s;

namespace {
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
  xlnt_index_t contests;
  xlnt_index_t login;
  xlnt_index_t password;
  xlnt_index_t name;
};

struct SubsheetOfInterest {
  xlnt::worksheet worksheet;
  ColumnsOfInterest cols;
  std::vector<xlnt_index_t> rows;
};

namespace {

std::optional<SubsheetOfInterest> check_sheet(xlnt::worksheet ws) {
  auto first_row = ws.rows().front();
  std::optional<xlnt_index_t> contests, login, password, name;
  for (auto cell : first_row) {
    auto content = [&]() {
      auto raw = u8_to_u32(cell.to_string());
      auto trimmed = trim(std::u32string_view(raw));
      std::u32string res;
      res.reserve(trimmed.size());
      std::ranges::copy(trimmed | view_toupper<char32_t>, std::back_inserter(res));
      return res;
    }();
    xlnt_index_t col = cell.column_index();
    if (recognized_column_names::is_name(content))
      name.emplace(col);
    else if (recognized_column_names::is_login(content))
      login.emplace(col);
    else if (recognized_column_names::is_password(content))
      password.emplace(col);
    else if (recognized_column_names::is_contests(content))
      contests.emplace(col);
  }
  if (!contests || !login || !password || !name) return {};
  std::vector<xlnt_index_t> rows;
  xlnt_index_t name_v = *name;
  auto dims = ws.calculate_dimension();
  xlnt_index_t min_row = dims.top_left().row();
  xlnt_index_t max_row = ws.calculate_dimension().bottom_right().row();
  if (max_row > min_row)
    for (auto cell :
         xlnt::range(ws, xlnt::range_reference({name_v}, min_row + 1, {name_v}, max_row),
                     xlnt::major_order::column, true)[0]) {
      rows.emplace_back(cell.row());
    }
  return {{ws, {*contests, *login, *password, name_v}, std::move(rows)}};
}

std::generator<xlnt::worksheet> worksheets_starting_with_active(xlnt::workbook wb) {
  auto active = wb.active_sheet();
  co_yield active;
  for (auto ws : wb) {
    if (ws != active) co_yield ws;
  }
}

std::expected<xlnt::workbook, std::string> open_wb(std::filesystem::path path) {
  xlnt::workbook wb;
  try {
    wb.load(xlnt::path{path});
  } catch (xlnt::exception exc) {
    std::ostringstream out;
    std::ranges::copy(u8"Не открылся файл " | view_static_cast<char>,
                      std::ostream_iterator<char>(out));
    out << path << ": " << exc.what() << " (" << exc.message() << ")";
    return std::unexpected(std::move(out).str());
  }
  return {std::move(wb)};
}

std::optional<SubsheetOfInterest> check_wb(xlnt::workbook& wb) {
  for (auto ws : worksheets_starting_with_active(wb)) {
    auto result = check_sheet(ws);
    if (result) return result;
  }
  return {};
}

bool check_completeness(SubsheetOfInterest sheet) {
  for (auto row : sheet.rows)
    for (auto col : {sheet.cols.contests, sheet.cols.login, sheet.cols.password})
      if (sheet.worksheet.cell({col}, row) == nullptr) return false;
  return true;
}

std::string error_message_no_worksheets_matched(std::string_view filename) {
  std::ostringstream out;
  std::ranges::copy(u8"Ни одна страница не содержит нужных столбцов: " | view_static_cast<char>,
                    std::ostream_iterator<char>(out));
  out << filename;
  return std::move(out).str();
}

std::tuple<std::vector<xlnt::cell>, std::string, unsigned long long> extract_logins_to_fill(
    SubsheetOfInterest sheet) {
  auto ws = sheet.worksheet;
  std::string login_prefix = "user";
  unsigned long long login_highest = 0;
  std::vector<xlnt::cell> logins_to_fill;
  for (auto row : sheet.rows) {
    const auto login_cell = ws.cell({sheet.cols.login}, row);
    if (login_cell == nullptr) {
      logins_to_fill.push_back(login_cell);
      continue;
    }
    std::string login = login_cell.to_string();
    if (login.empty()) {
      logins_to_fill.push_back(login_cell);
      continue;
    }
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

std::vector<xlnt::cell> extract_passwords_to_fill(SubsheetOfInterest sheet) {
  return sheet.rows |
         std::views::transform([ws = sheet.worksheet, col = sheet.cols.password](xlnt_index_t ri) {
           return ws.cell({col}, ri);
         }) |
         std::views::cache_latest |
         std::views::filter([](xlnt::cell cell) { return cell != nullptr; }) |
         std::ranges::to<std::vector<xlnt::cell>>();
}

std::pair<std::vector<xlnt::cell>, std::optional<std::string>> extract_contests_to_fill(
    SubsheetOfInterest sheet) {
  std::pair<std::vector<xlnt::cell>, std::optional<std::string>> res;
  auto& [to_fill, content] = res;
  auto ws = sheet.worksheet;
  for (auto row : sheet.rows) {
    const auto cell = ws.cell({sheet.cols.contests}, row);
    if (cell == nullptr) {
      to_fill.push_back(cell);
      continue;
    }
    auto s = cell.to_string();
    if (s.empty()) {
      to_fill.push_back(cell);
      continue;
    }
    if (!content) content.emplace(std::move(s));
  }
  return res;
}

}  // namespace

XLSheet::XLSheet(xlnt::path path) : filename{path} {}

std::expected<XLSheet::Info, std::string> XLSheet::Check() const {
  using ret_t = std::expected<XLSheet::Info, std::string>;
  return open_wb(filename.string()).and_then([this](xlnt::workbook wb) -> ret_t {
    auto sheet = check_wb(wb);
    if (!sheet) return std::unexpected(error_message_no_worksheets_matched(filename.string()));
    return {{sheet->rows.size(), check_completeness(*sheet)}};
  });
}

std::expected<void, std::string> XLSheet::Complete() const {
  auto wb = open_wb(filename.string());
  if (!wb) return std::unexpected(wb.error());
  auto sheet = check_wb(*wb);
  if (!sheet) return std::unexpected(error_message_no_worksheets_matched(filename.string()));

  // TODO
  // instead of what is below, use the new extract_*_to_fill functions to make the implementation
  // clean.

  // std::optional<xlnt::cell> contests;
  // for (auto row : sheet->rows) {
  //   auto contest = ws.cell({sheet->cols.contests}, row);
  //   if (!(contest == nullptr) && !contests) contests = contest;
  //   const auto login_cell = ws.cell({sheet->cols.login}, row);
  //   if (login_cell == nullptr) {
  //     logins_to_fill.push_back(login_cell);
  //     continue;
  //   }
  //   std::string login = login_cell.to_string();
  //   auto pos = login.find_last_not_of("0123456789");
  //   if (pos == std::string::npos || pos + 1 == login.size()) continue;
  //   unsigned long long number;
  //   auto parsed = std::from_chars(login.data() + pos + 1, login.data() + login.size(), number);
  //   if (parsed.ec != std::errc{} || parsed.ptr != login.data() + login.size()) continue;
  //   if (number >= login_highest) {
  //     login_highest = number;
  //     login_prefix = login.substr(0, pos + 1);
  //   }
  // }
  // const auto missing_logins = std::ranges::count_if(
  //     sheet->rows, [&](auto row) { return ws.cell({sheet->cols.login}, row) == nullptr; });
  // // if (static_cast<unsigned long long>(missing_logins) >
  // //     std::numeric_limits<unsigned long long>::max() - highest)
  // //   return std::unexpected("Login numbering would overflow.");
  // for (int i 
  // PasswordGenerator passwords;
  // for (auto row : sheet->rows) {
  //   auto login = ws.cell({sheet->cols.login}, row);
  //   if (login == nullptr) login.value(prefix + std::to_string(++highest));
  //   auto password = ws.cell({sheet->cols.password}, row);
  //   if (password == nullptr) password.value(passwords(12));
  //   auto contest = ws.cell({sheet->cols.contests}, row);
  //   if (contest == nullptr && contests) contest.value(*contests);
  // }
  // try {
  //   wb->save(filename);
  // } catch (const std::exception& exc) {
  //   return std::unexpected("Could not save workbook: "s + exc.what());
  // }
  // if (!contests) return std::unexpected("No non-empty contest cells were found.");
  // return {};
}

std::expected<std::string, std::string> XLSheet::FormatForCodeforcesUTF8() const {
  auto wb = open_wb(filename.string());
  if (!wb) return std::unexpected(wb.error());
  auto sheet = check_wb(*wb);
  if (!sheet) return std::unexpected(error_message_no_worksheets_matched(filename.string()));
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
      result << sheet->worksheet.cell({col}, row).to_string();
    }
  }
  return std::move(result).str();
}

const xlnt::path& XLSheet::Filename() const { return filename; }
