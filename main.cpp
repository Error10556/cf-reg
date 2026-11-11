#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <iterator>
#include <limits>
#include <locale>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <xlnt/xlnt.hpp>

#include "xlnt/utils/exceptions.hpp"


int main(int argc, char** argv) {
  using namespace std::literals;
  using namespace std::placeholders;
  if (argc == 0) return 1;
  std::span<char*> argspan{argv + 1, static_cast<size_t>(argc - 1)};
  for (auto cstr : argspan) {
    std::filesystem::path s{cstr};
    //
  }
}
