// SPDX-License-Identifier: 0BSD
#include "passwords.hpp"

#include <chrono>
#include <string_view>

PasswordGenerator::PasswordGenerator()
    : engine(static_cast<std::mt19937::result_type>(
          std::chrono::steady_clock::now().time_since_epoch().count())) {}

std::string PasswordGenerator::operator()(size_t len) {
  constexpr std::string_view alphabet = "abcdefghkmnpqrstwxyz123456789";
  std::uniform_int_distribution<size_t> choose(0, alphabet.size() - 1);
  std::string result(len, '\0');
  for (auto& ch : result) ch = alphabet[choose(engine)];
  return result;
}
