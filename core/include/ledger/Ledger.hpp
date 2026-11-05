#pragma once

#include "ledger/Date.hpp"
#include "ledger/EventLog.hpp"
#include "ledger/Invoice.hpp"
#include "ledger/Money.hpp"
#include "ledger/PaymentRule.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <vector>

namespace ledger {

struct PaymentEvent {
    int invoiceId;
    Money amount;
    Date when;
};

// Thread-safe: readers share a std::shared_mutex, writers take it exclusively.
class Ledger {
public:
    Ledger() = default;
    Ledger(const Ledger&) = delete;
    Ledger& operator=(const Ledger&) = delete;

    void addRule(std::unique_ptr<PaymentRule> rule);
    void addInvoice(Invoice invoice);
    void applyPayment(int invoiceId, Money amount, Date when);  // throws LedgerError

    [[nodiscard]] std::optional<Invoice> find(int invoiceId) const;
    [[nodiscard]] std::vector<Invoice> invoices() const;             // sorted by due date
    [[nodiscard]] std::vector<Invoice> overdueAsOf(Date asOf) const;  // sorted by due date
    [[nodiscard]] Money outstanding() const;
    [[nodiscard]] std::size_t paymentCount() const;
    [[nodiscard]] std::vector<PaymentEvent> takeEvents();

private:
    std::vector<std::unique_ptr<PaymentRule>> rules_;
    std::vector<Invoice> invoices_;
    EventLog<PaymentEvent> events_;
    mutable std::shared_mutex mutex_;
};

}  // namespace ledger
