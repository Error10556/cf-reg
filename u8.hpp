#pragma once
#include <ranges>
#include <string>

std::u32string u8_to_u32(std::string_view);
std::string u32_to_u8(std::u32string_view);

namespace {

constexpr inline char32_t ru_en_toupper(char32_t ch) noexcept {
  if (U'a' <= ch && ch <= U'z') return ch + U'A' - U'a';
  if (U'а' <= ch && ch <= U'я') return ch + U'А' - U'а';
  if (ch == U'ё') return U'Ё';
  return ch;
}

constexpr inline char en_toupper(char ch) noexcept {
  return 'a' <= ch && ch <= 'z' ? ch + 'A' - 'a' : ch;
}

template <class TChar>
constexpr auto view_toupper = std::views::transform([](TChar ch) {
  if constexpr (std::is_same_v<TChar, char>)
    return en_toupper(ch);
  else
    return ru_en_toupper(ch);
});

template <class TCharTo>
constexpr auto view_static_cast =
    std::views::transform([](auto ch) { return static_cast<TCharTo>(ch); });

}  // namespace

// template <std::ranges::range Range, class TChar>
//   requires std::is_same_v<std::ranges::range_value_t<Range>, TChar>
// static inline std::basic_ostream<TChar>& operator<<(std::basic_ostream<TChar>& out, Range&& range) {
//   std::ranges::copy(range, std::ostream_iterator(out));
//   return out;
// }

template <class TChar>
std::basic_string_view<TChar> trim(std::basic_string_view<TChar>);

extern template std::basic_string_view<char> trim<char>(std::basic_string_view<char>);
extern template std::basic_string_view<char32_t> trim<char32_t>(std::basic_string_view<char32_t>);

template <class TCharTo, class TCharFrom>
std::basic_string<TCharTo> static_char_cast(std::basic_string_view<TCharFrom> sv) {
  std::basic_string<TCharTo> res;
  res.reserve(sv.size());
  for (auto ch : sv) res.push_back(static_cast<TCharTo>(ch));
  return res;
}
