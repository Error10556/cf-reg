#include "u8.hpp"

#include <cstdint>
#include <limits>
#include <cctype>

constexpr unsigned char u8_char_size[]{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,

    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,

    4, 4, 4, 4, 4, 4, 4, 4,

    0, 0, 0, 0, 0, 0, 0, 0,
};
static_assert(std::size(u8_char_size) == 128);

constexpr uint32_t min_value_for_u8_width[]{
    0, 0, 1U << 7, 1U << 11, 1U << 16,
};
static_assert(std::size(min_value_for_u8_width) == 5);

std::u32string u8_to_u32(std::string_view s) {
  std::u32string res;
  size_t n = s.size();
  size_t i = 0;
  while (i < n) {
    signed char cur = s[i];
    if (cur >= 0) {
      res.push_back(static_cast<unsigned char>(cur));
      ++i;
      continue;
    }
    unsigned char w = u8_char_size[static_cast<unsigned char>(s[i]) - 128];
    if (!w) {
      ++i;
      res.push_back(static_cast<char32_t>(-1));
      continue;
    }
    size_t end = i + w;
    if (end > n) {
      res.push_back(static_cast<char32_t>(-1));
      return res;
    }
    uint32_t val = static_cast<uint32_t>(s[i]) & ((1U << (7 - w)) - 1);
    for (++i; i < end; ++i) {
      uint32_t ch = static_cast<unsigned char>(s[i]);
      if ((ch & 0xc0) != 0x80) {
        val = std::numeric_limits<uint32_t>::max();
        i = end;
        break;
      }
      val = (val << 6) | (ch ^ 0x80);
    }
    if (val < min_value_for_u8_width[w]) val = std::numeric_limits<uint32_t>::max();
    res.push_back(val);
  }
  return res;
}

template <class TChar>
std::basic_string_view<TChar> trim(std::basic_string_view<TChar> sv) {
  auto is_space = [](TChar ch) {
    if (ch < 0) return false;
    return ch <= 127 && std::isspace(static_cast<int>(ch));
  };
  size_t n = sv.size();
  size_t start = 0;
  for (; start < n && is_space(sv[start]); ++start);
  if (start == n) return std::basic_string_view<TChar>();
  size_t end = n;
  for (; end > start && (is_space(sv[end - 1])); --end);
  return sv.substr(start, end - start);
}

template std::basic_string_view<char> trim<char>(std::basic_string_view<char>);
template std::basic_string_view<char32_t> trim<char32_t>(std::basic_string_view<char32_t>);
