#undef NDEBUG  // keep assert() active in Release builds too
#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "ledger/Ledger.hpp"

using namespace ledger;

namespace {

template <typename Fn>
bool throwsLedgerError(Fn&& fn) {
    try {
        fn();
    } catch (const LedgerError&) {
        return true;
    }
    return false;
}

void testMoney() {
    const Money a{1050};
    const Money b{295};
    assert((a + b) == Money{1345});
    assert((a - b).cents() == 755);
    assert(b < a);
    assert(Money{123456789}.toString() == "$1,234,567.89");
    assert(Money{-550}.toString() == "-$5.50");
    assert(Money{5}.toString() == "$0.05");
    assert(Money::fromDollars(12.34) == Money{1234});
}

void testInvoiceStatus() {
    Invoice inv{1, "Acme", Money{10000}, makeDate(2026, 1, 31)};
    assert(inv.status() == InvoiceStatus::Open);

    inv.applyPayment(Money{4000});
    assert(inv.status() == InvoiceStatus::PartiallyPaid);
    assert(throwsLedgerError([&] { inv.applyPayment(Money{7000}); }));

    inv.applyPayment(Money{6000});
    assert(inv.status() == InvoiceStatus::Paid);
    assert(inv.outstanding() == Money{});
}

void testRulesRejectBadPayments() {
    Ledger book;
    book.addRule(std::make_unique<NoOverpayment>());
    book.addRule(std::make_unique<MinimumInstallment>(Money{2500}));
    book.addInvoice(Invoice{1, "Acme", Money{10000}, makeDate(2026, 1, 31)});

    const auto when = makeDate(2026, 1, 5);
    assert(throwsLedgerError([&] { book.applyPayment(1, Money{10001}, when); }));  // overpayment
    assert(throwsLedgerError([&] { book.applyPayment(1, Money{1000}, when); }));   // below minimum
    assert(throwsLedgerError([&] { book.applyPayment(42, Money{2500}, when); }));  // unknown id
    assert(book.paymentCount() == 0);

    book.applyPayment(1, Money{9000}, when);
    book.applyPayment(1, Money{1000}, when);  // final balance may be below the minimum
    assert(book.outstanding() == Money{});
    assert(book.paymentCount() == 2);
}

void testLedgerOutstandingAndEvents() {
    Ledger book;
    book.addInvoice(Invoice{1, "Acme", Money{10000}, makeDate(2026, 1, 31)});
    book.addInvoice(Invoice{2, "Globex", Money{5000}, makeDate(2026, 2, 28)});
    assert(book.outstanding() == Money{15000});

    book.applyPayment(2, Money{5000}, makeDate(2026, 1, 10));
    assert(book.outstanding() == Money{10000});

    auto events = book.takeEvents();
    assert(events.size() == 1 && events[0].invoiceId == 2);
    assert(book.takeEvents().empty());
}

void testLookupAndDuplicates() {
    Ledger book;
    book.addInvoice(Invoice{7, "Initech", Money{100}, makeDate(2026, 3, 1)});

    const auto found = book.find(7);
    assert(found.has_value() && found->customer() == "Initech");
    assert(!book.find(99).has_value());
    assert(throwsLedgerError([&] { book.addInvoice(Invoice{7, "Dup", Money{100}, makeDate(2026, 3, 1)}); }));
}

void testOverdueAsOf() {
    Ledger book;
    book.addInvoice(Invoice{1, "Late", Money{300}, makeDate(2026, 5, 10)});
    book.addInvoice(Invoice{2, "Early", Money{100}, makeDate(2026, 2, 1)});
    book.addInvoice(Invoice{3, "Future", Money{200}, makeDate(2026, 12, 1)});
    book.addInvoice(Invoice{4, "Settled", Money{50}, makeDate(2026, 2, 1)});
    book.applyPayment(4, Money{50}, makeDate(2026, 1, 20));

    const auto overdue = book.overdueAsOf(makeDate(2026, 6, 1));
    assert(overdue.size() == 2);
    assert(overdue[0].id() == 2);  // earliest due date first
    assert(overdue[1].id() == 1);
}

void testEventLogMovesEvents() {
    EventLog<std::string> log;
    std::string text = "payment-1";
    log.record(std::move(text));

    EventLog<std::string> moved = std::move(log);
    assert(moved.size() == 1);
    assert(moved.events().front() == "payment-1");

    auto taken = moved.take();
    assert(taken.size() == 1 && moved.size() == 0);
}

void testConcurrentReads() {
    Ledger book;
    for (int i = 1; i <= 50; ++i) {
        book.addInvoice(Invoice{i, "Cust", Money{1000}, makeDate(2026, 3, 1)});
    }
    book.applyPayment(1, Money{400}, makeDate(2026, 2, 1));

    const Money expected{50000 - 400};
    std::atomic<int> mismatches{0};
    std::vector<std::thread> readers;
    for (int t = 0; t < 4; ++t) {
        readers.emplace_back([&] {
            for (int i = 0; i < 200; ++i) {
                if (book.outstanding() != expected) ++mismatches;
                if (!book.find(25).has_value()) ++mismatches;
            }
        });
    }
    for (auto& reader : readers) reader.join();
    assert(mismatches == 0);
}

}  // namespace

int main() {
    testMoney();
    testInvoiceStatus();
    testRulesRejectBadPayments();
    testLedgerOutstandingAndEvents();
    testLookupAndDuplicates();
    testOverdueAsOf();
    testEventLogMovesEvents();
    testConcurrentReads();
    std::cout << "All ledger tests passed.\n";
    return 0;
}
