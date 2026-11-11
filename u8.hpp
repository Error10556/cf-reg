#pragma once
#include <string>
#include <ranges>

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

}  // namespace

template <class TChar>
std::basic_string_view<TChar> trim(std::basic_string_view<TChar>);

extern template std::basic_string_view<char> trim<char>(std::basic_string_view<char>);
extern template std::basic_string_view<char32_t> trim<char32_t>(std::basic_string_view<char32_t>);
