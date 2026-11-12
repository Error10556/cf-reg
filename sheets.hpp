#pragma once

#include <span>
#include <vector>

#include "xlmodel.hpp"

std::vector<XLSheet> sheets_from_paths(std::span<char* const> paths);
