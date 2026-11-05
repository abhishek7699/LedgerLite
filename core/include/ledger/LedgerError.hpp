#pragma once

#include <stdexcept>

namespace ledger {

// Thrown for every rule violation or invalid operation in the core.
class LedgerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}  // namespace ledger
