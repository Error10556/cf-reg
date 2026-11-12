#include <iostream>
#include <span>
#include <sstream>
#include <string>

#include "sheets.hpp"

int main(int argc, char** argv) {
  if (argc == 0) return 1;
  std::span<char*> argspan{argv + 1, static_cast<size_t>(argc - 1)};
  auto sheets = sheets_from_paths(argspan);
  if (sheets.empty()) {
    std::cerr << "Usage: " << argv[0] << " FILE.xlsx [FILE.xlsx ...]\n";
    return 1;
  }

  auto read_number = [](int& number) {
    std::string line;
    if (!std::getline(std::cin, line)) return false;
    std::istringstream input(line);
    char extra;
    if (!(input >> number) || (input >> extra)) number = -2;
    return true;
  };

  while (true) {
    std::cout << "\nWorkbooks:\n";
    for (size_t i = 0; i < sheets.size(); ++i)
      std::cout << i << ": " << sheets[i].Filename().string() << '\n';
    std::cout << "Select workbook index (-1 to exit): " << std::flush;
    int index;
    if (!read_number(index) || index == -1) return 0;
    if (index < 0 || static_cast<size_t>(index) >= sheets.size()) {
      std::cerr << "Invalid workbook index.\n";
      continue;
    }

    auto& sheet = sheets[static_cast<size_t>(index)];
    std::cout << "1: Check\n2: Complete and save\n3: Format for Codeforces\n"
                 "0: Back\nSelect action: " << std::flush;
    int action;
    if (!read_number(action)) return 0;
    try {
      switch (action) {
        case 0:
          break;
        case 1: {
          auto result = sheet.Check();
          if (result)
            std::cout << "Participants: " << result->participant_count
                      << "\nComplete: " << (result->complete ? "yes" : "no") << '\n';
          else
            std::cerr << "Error: " << result.error() << '\n';
          break;
        }
        case 2: {
          auto result = sheet.Complete();
          if (result)
            std::cout << "Workbook completed and saved.\n";
          else
            std::cerr << "Error: " << result.error() << '\n';
          break;
        }
        case 3: {
          auto result = sheet.FormatForCodeforcesUTF8();
          if (result)
            std::cout << *result << '\n';
          else
            std::cerr << "Error: " << result.error() << '\n';
          break;
        }
        default:
          std::cerr << "Invalid action.\n";
      }
    } catch (const std::exception& exc) {
      std::cerr << "Error: " << exc.what() << '\n';
    }
  }
}
