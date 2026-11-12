// SPDX-License-Identifier: 0BSD
#pragma once
#include <cstdint>
#include <random>
#include <string>

class PasswordGenerator {
public:
  PasswordGenerator();
  std::string operator()(size_t len);

private:
  std::mt19937 engine;
};
