#pragma once
#include <cstdint>
#include <random>
#include <string>

class PasswordGenerator {
public:
  PasswordGenerator(); // TODO: initialize mt19937 with chrono::steady_clock::now().count()
  std::string operator()(size_t len);
  // ^ TODO: return std::string with 'len' random characters and
  // digits. Choose from a-z without l, i, j, I, J, o, 0, O, v, u

private:
  // TODO use mt19937
};
