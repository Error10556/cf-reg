#include "xlmodel.hpp"

#include <algorithm>
#include <filesystem>
#include <generator>
#include <iterator>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

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
      auto trimmed = trim(raw.subview());
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

std::expected<std::optional<SubsheetOfInterest>, std::string> check_wb(std::filesystem::path path) {
  using ret_t = std::expected<std::optional<SubsheetOfInterest>, std::string>;
  return open_wb(path).and_then([](xlnt::workbook wb) -> ret_t {
    for (auto ws : worksheets_starting_with_active(wb)) {
      auto maybe_res = check_sheet(ws);
      if (maybe_res) return {std::move(maybe_res)};
    }
    return {};
  });
}

bool check_completeness(SubsheetOfInterest sheet) {
  auto ws = sheet.worksheet;
  // Hint: check like this: ws.cell({1}, {1}) == nullptr;
  // TODO:
  // In all columns in sheet.cols, except 'name', check that all cells in rows in sheet.rows
  // are not null.
}

}  // namespace

XLSheet::XLSheet(xlnt::path path) : filename{path} {}

std::expected<XLSheet::Info, std::string> XLSheet::Check() const {
  // TODO:
  // use open_wb.
  // call check_wb.
  // if returned an empty optional, report that no worksheet matched.
  // Otherwise, report check_completeness as completeness and rows.size() as row count.
}

std::expected<void, std::string> XLSheet::Complete() const {
  // TODO:
  // use open_wb.
  // call check_wb.
  // if returned an empty optional, report that no worksheet matched.
  // Otherwise, fill in the passwords, logins, and contest lists on appropriate rows.
  // Passwords are random, use passswords.hpp.
  // Logins are of the form "something1234"; find the one with the highest number at the end and
  // fill the empty cells with increments of that number and the same prefix.
  // Do not consider existing logins of the wrong format.
  // If there are no logins of the right format, use the prefix "user" and start with 1.
  // Copy contents of one of the non-empty contest cells.
  // Save the workbook if no errors so far.
  // If there are no non-empty contest cells, report that as a std::unexpected, but still save the
  // workbook.
}

std::expected<std::string, std::string> XLSheet::FormatForCodeforcesUTF8() const {
  // TODO:
  // Codeforces expects the following format for each user:
  //
  // contests | login | password | name
  // contests | login | password | name
  // ...
  //
  // extract the data and return the '\n'-separated lines as a string.
}

const xlnt::path& XLSheet::Filename() const {
  return filename;
}
