#include <filesystem>
#include <xlnt/xlnt.hpp>

#include "u8.hpp"
#include "xlmodel.hpp"

int main(int argc, char** argv) {
  using namespace std::literals;
  using namespace std::placeholders;
  if (argc == 0) return 1;
  std::span<char*> argspan{argv + 1, static_cast<size_t>(argc - 1)};
  std::vector<XLSheet> sheets;
  for (auto cstr : argspan) {
    std::filesystem::path s{cstr};
    auto ext_u8 = s.extension().string();
    if (std::ranges::equal(ext_u8 | view_toupper<char>, ".XLSX"))
      sheets.emplace_back(xlnt::path{s});
  }
  // TODO
  // make a simple cli that calls methods of sheets[i], first selecting the index i.
  while (true) {
    // TODO
    // index = -1 to exit
  }
}
