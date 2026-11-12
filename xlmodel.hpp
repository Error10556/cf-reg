// SPDX-License-Identifier: 0BSD
#pragma once
#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>

class XLSheet {
public:
  struct Info {
    size_t participant_count;
    bool complete;
  };
  XLSheet(std::filesystem::path path);
  std::expected<Info, std::string> Check() const;
  std::expected<void, std::string> Complete() const;
  std::expected<std::string, std::string> FormatForCodeforcesUTF8() const;
  const std::filesystem::path& Filename() const;

private:
  std::filesystem::path filename;
};
