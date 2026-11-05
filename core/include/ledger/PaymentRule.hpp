#pragma once

#include "ledger/Invoice.hpp"
#include "ledger/LedgerError.hpp"
#include "ledger/Money.hpp"

#include <string>

namespace ledger {

// Polymorphic policy checked before every payment is applied.
class PaymentRule {
public:
    virtual ~PaymentRule() = default;

    // Throws LedgerError if the payment must not be applied.
    virtual void validate(const Invoice& invoice, Money amount) const = 0;
    [[nodiscard]] virtual std::string name() const = 0;
};

class NoOverpayment final : public PaymentRule {
public:
    void validate(const Invoice& invoice, Money amount) const override {
        if (amount > invoice.outstanding())
            throw LedgerError("overpayment on invoice " + std::to_string(invoice.id()));
    }
    [[nodiscard]] std::string name() const override { return "NoOverpayment"; }
};

class MinimumInstallment final : public PaymentRule {
public:
    explicit MinimumInstallment(Money minimum) : minimum_{minimum} {}

    void validate(const Invoice& invoice, Money amount) const override {
        // The final payment may settle a balance smaller than the minimum.
        if (amount < minimum_ && amount != invoice.outstanding())
            throw LedgerError("payment below minimum installment " + minimum_.toString());
    }
    [[nodiscard]] std::string name() const override { return "MinimumInstallment"; }

private:
    Money minimum_;
};

}  // namespace ledger
