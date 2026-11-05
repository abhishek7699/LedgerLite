#pragma once

#include "ledger/Date.hpp"
#include "ledger/LedgerError.hpp"
#include "ledger/Money.hpp"

#include <string>
#include <utility>

namespace ledger {

enum class InvoiceStatus { Open, PartiallyPaid, Paid };

[[nodiscard]] inline std::string toString(InvoiceStatus status) {
    switch (status) {
        case InvoiceStatus::Open:          return "Open";
        case InvoiceStatus::PartiallyPaid: return "Partial";
        case InvoiceStatus::Paid:          return "Paid";
    }
    return "?";
}

class Invoice {
public:
    Invoice(int id, std::string customer, Money total, Date dueDate)
        : id_{id}, customer_{std::move(customer)}, total_{total}, dueDate_{dueDate} {
        if (total <= Money{}) throw LedgerError("invoice total must be positive");
    }

    [[nodiscard]] int id() const noexcept { return id_; }
    [[nodiscard]] const std::string& customer() const noexcept { return customer_; }
    [[nodiscard]] Money total() const noexcept { return total_; }
    [[nodiscard]] Money paid() const noexcept { return paid_; }
    [[nodiscard]] Money outstanding() const noexcept { return total_ - paid_; }
    [[nodiscard]] Date dueDate() const noexcept { return dueDate_; }
    [[nodiscard]] InvoiceStatus status() const noexcept { return status_; }

    void applyPayment(Money amount) {
        if (amount <= Money{} || amount > outstanding())
            throw LedgerError("payment out of range for invoice " + std::to_string(id_));
        paid_ += amount;
        status_ = (paid_ == total_) ? InvoiceStatus::Paid : InvoiceStatus::PartiallyPaid;
    }

private:
    int id_;
    std::string customer_;
    Money total_;
    Money paid_{};
    Date dueDate_;
    InvoiceStatus status_{InvoiceStatus::Open};
};

}  // namespace ledger
