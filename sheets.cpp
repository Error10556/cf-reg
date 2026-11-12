#include "sheets.hpp"

#include <algorithm>
#include <filesystem>
#include <string_view>

#include "u8.hpp"

std::vector<XLSheet> sheets_from_paths(std::span<char* const> paths) {
  std::vector<XLSheet> sheets;
  for (auto cstr : paths) {
    std::filesystem::path path{cstr};
    auto extension = path.extension().string();
    if (std::ranges::equal(extension | view_toupper<char>, std::string_view(".XLSX")))
      sheets.emplace_back(xlnt::path{path});
  }
  return sheets;
}
