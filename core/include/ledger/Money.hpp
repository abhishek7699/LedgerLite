#pragma once

#include <cmath>
#include <cstdint>
#include <string>

namespace ledger {

// Money as integer cents: exact arithmetic, no floating-point drift.
class Money {
public:
    constexpr Money() noexcept = default;
    constexpr explicit Money(std::int64_t cents) noexcept : cents_{cents} {}

    static Money fromDollars(double dollars) noexcept {
        return Money{static_cast<std::int64_t>(std::llround(dollars * 100.0))};
    }

    [[nodiscard]] constexpr std::int64_t cents() const noexcept { return cents_; }
    [[nodiscard]] std::string toString() const;  // e.g. "$1,234.56"

    constexpr Money& operator+=(Money rhs) noexcept { cents_ += rhs.cents_; return *this; }
    constexpr Money& operator-=(Money rhs) noexcept { cents_ -= rhs.cents_; return *this; }

    friend constexpr Money operator+(Money lhs, Money rhs) noexcept { return lhs += rhs; }
    friend constexpr Money operator-(Money lhs, Money rhs) noexcept { return lhs -= rhs; }

    // One defaulted three-way comparison gives ==, !=, <, <=, >, >=.
    constexpr auto operator<=>(const Money&) const noexcept = default;

private:
    std::int64_t cents_{};
};

}  // namespace ledger
