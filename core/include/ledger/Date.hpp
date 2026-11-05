#pragma once

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace ledger {

using Date = std::chrono::year_month_day;

inline Date makeDate(int year, unsigned month, unsigned day) {
    return Date{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};
}

inline std::string toString(const Date& date) {
    std::ostringstream out;
    out << static_cast<int>(date.year()) << '-'
        << std::setw(2) << std::setfill('0') << static_cast<unsigned>(date.month()) << '-'
        << std::setw(2) << std::setfill('0') << static_cast<unsigned>(date.day());
    return out.str();
}

}  // namespace ledger
