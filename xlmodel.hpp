#pragma once
#include <expected>
#include <xlnt/xlnt.hpp>

using xlnt_index_t = xlnt::column_t::index_t;

class XLSheet {
public:
  struct Info {
    size_t participant_count;
    bool complete;
  };
  XLSheet(xlnt::path path);
  std::expected<Info, std::string> Check() const;
  std::expected<void, std::string> Complete() const;
  std::expected<std::string, std::string> FormatForCodeforcesUTF8() const;
  const xlnt::path& Filename() const;

private:
  xlnt::path filename;
};
