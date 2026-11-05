#include "ledger/Money.hpp"

#include <cstddef>

namespace ledger {

std::string Money::toString() const {
    const std::int64_t absCents = cents_ < 0 ? -cents_ : cents_;

    std::string dollars = std::to_string(absCents / 100);
    for (auto pos = static_cast<std::ptrdiff_t>(dollars.size()) - 3; pos > 0; pos -= 3) {
        dollars.insert(static_cast<std::size_t>(pos), ",");
    }

    std::string cents = std::to_string(absCents % 100);
    if (cents.size() < 2) cents.insert(0, "0");

    return std::string{cents_ < 0 ? "-$" : "$"} + dollars + "." + cents;
}

}  // namespace ledger
