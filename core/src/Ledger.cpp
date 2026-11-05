#include "ledger/Ledger.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <mutex>
#include <numeric>
#include <ranges>

namespace ledger {

void Ledger::addRule(std::unique_ptr<PaymentRule> rule) {
    if (!rule) throw LedgerError("null payment rule");
    std::unique_lock lock{mutex_};
    rules_.push_back(std::move(rule));
}

void Ledger::addInvoice(Invoice invoice) {
    std::unique_lock lock{mutex_};
    const bool duplicate = std::ranges::any_of(
        invoices_, [&](const Invoice& existing) { return existing.id() == invoice.id(); });
    if (duplicate) throw LedgerError("duplicate invoice " + std::to_string(invoice.id()));
    invoices_.push_back(std::move(invoice));
}

void Ledger::applyPayment(int invoiceId, Money amount, Date when) {
    std::unique_lock lock{mutex_};
    auto it = std::ranges::find(invoices_, invoiceId, &Invoice::id);
    if (it == invoices_.end()) throw LedgerError("unknown invoice " + std::to_string(invoiceId));

    // Validate everything before mutating, so a rejected payment changes nothing.
    for (const auto& rule : rules_) rule->validate(*it, amount);
    it->applyPayment(amount);
    events_.record(PaymentEvent{invoiceId, amount, when});
}

std::optional<Invoice> Ledger::find(int invoiceId) const {
    std::shared_lock lock{mutex_};
    const auto it = std::ranges::find(invoices_, invoiceId, &Invoice::id);
    if (it == invoices_.end()) return std::nullopt;
    return *it;
}

std::vector<Invoice> Ledger::invoices() const {
    std::shared_lock lock{mutex_};
    std::vector<Invoice> result(invoices_);
    std::ranges::sort(result, std::less<>{}, &Invoice::dueDate);
    return result;
}

std::vector<Invoice> Ledger::overdueAsOf(Date asOf) const {
    std::shared_lock lock{mutex_};
    auto overdue = invoices_ | std::views::filter([asOf](const Invoice& inv) {
        return inv.status() != InvoiceStatus::Paid && inv.dueDate() < asOf;
    });
    std::vector<Invoice> result;
    std::ranges::copy(overdue, std::back_inserter(result));
    std::ranges::sort(result, std::less<>{}, &Invoice::dueDate);
    return result;
}

Money Ledger::outstanding() const {
    std::shared_lock lock{mutex_};
    return std::accumulate(invoices_.begin(), invoices_.end(), Money{},
                           [](Money sum, const Invoice& inv) { return sum + inv.outstanding(); });
}

std::size_t Ledger::paymentCount() const {
    std::shared_lock lock{mutex_};
    return events_.size();
}

std::vector<PaymentEvent> Ledger::takeEvents() {
    std::unique_lock lock{mutex_};
    return events_.take();
}

}  // namespace ledger
