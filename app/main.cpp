#include <QApplication>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <chrono>
#include <memory>

#include "ledger/Ledger.hpp"

namespace {

ledger::Date today() {
    return std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
}

ledger::Date daysFromToday(int days) {
    return ledger::Date{std::chrono::sys_days{today()} + std::chrono::days{days}};
}

}  // namespace

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ledger::Ledger& ledger) : ledger_{ledger} {
        setWindowTitle("LedgerLite");
        resize(820, 420);

        table_ = new QTableWidget(0, 7, this);  // Qt parent owns the widget
        table_->setHorizontalHeaderLabels({"ID", "Customer", "Due", "Total", "Paid", "Outstanding", "Status"});
        table_->horizontalHeader()->setStretchLastSection(true);
        table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->setSelectionMode(QAbstractItemView::SingleSelection);

        auto* addButton = new QPushButton("Add Invoice", this);
        auto* payButton = new QPushButton("Apply Payment", this);
        totalLabel_ = new QLabel(this);

        auto* buttons = new QHBoxLayout;
        buttons->addWidget(addButton);
        buttons->addWidget(payButton);
        buttons->addStretch();
        buttons->addWidget(totalLabel_);

        auto* layout = new QVBoxLayout;
        layout->addWidget(table_);
        layout->addLayout(buttons);

        auto* central = new QWidget(this);
        central->setLayout(layout);
        setCentralWidget(central);

        connect(addButton, &QPushButton::clicked, this, &MainWindow::addInvoice);
        connect(payButton, &QPushButton::clicked, this, &MainWindow::applyPayment);
        refresh();
    }

private slots:
    void addInvoice() {
        bool ok = false;
        const QString customer =
            QInputDialog::getText(this, "Add Invoice", "Customer:", QLineEdit::Normal, {}, &ok);
        if (!ok || customer.trimmed().isEmpty()) return;

        const double amount =
            QInputDialog::getDouble(this, "Add Invoice", "Amount ($):", 0.0, 0.01, 1e9, 2, &ok);
        if (!ok) return;

        try {
            ledger_.addInvoice(ledger::Invoice{nextId_++, customer.toStdString(),
                                               ledger::Money::fromDollars(amount), daysFromToday(30)});
        } catch (const ledger::LedgerError& e) {
            QMessageBox::warning(this, "Invoice rejected", QString::fromStdString(e.what()));
        }
        refresh();
    }

    void applyPayment() {
        const int row = table_->currentRow();
        if (row < 0) {
            QMessageBox::information(this, "Apply Payment", "Select an invoice first.");
            return;
        }
        const int id = table_->item(row, 0)->text().toInt();

        bool ok = false;
        const double amount =
            QInputDialog::getDouble(this, "Apply Payment", "Amount ($):", 0.0, 0.01, 1e9, 2, &ok);
        if (!ok) return;

        try {
            ledger_.applyPayment(id, ledger::Money::fromDollars(amount), today());
        } catch (const ledger::LedgerError& e) {
            QMessageBox::warning(this, "Payment rejected", QString::fromStdString(e.what()));
        }
        refresh();
    }

private:
    void refresh() {
        const auto invoices = ledger_.invoices();
        table_->setRowCount(static_cast<int>(invoices.size()));
        for (int row = 0; row < static_cast<int>(invoices.size()); ++row) {
            const auto& inv = invoices[static_cast<std::size_t>(row)];
            const QStringList cells{
                QString::number(inv.id()),
                QString::fromStdString(inv.customer()),
                QString::fromStdString(ledger::toString(inv.dueDate())),
                QString::fromStdString(inv.total().toString()),
                QString::fromStdString(inv.paid().toString()),
                QString::fromStdString(inv.outstanding().toString()),
                QString::fromStdString(ledger::toString(inv.status())),
            };
            for (int col = 0; col < cells.size(); ++col) {
                table_->setItem(row, col, new QTableWidgetItem(cells[col]));
            }
        }
        totalLabel_->setText("Total outstanding: " + QString::fromStdString(ledger_.outstanding().toString()));
    }

    ledger::Ledger& ledger_;
    QTableWidget* table_ = nullptr;
    QLabel* totalLabel_ = nullptr;
    int nextId_ = 1;
};

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    ledger::Ledger book;
    book.addRule(std::make_unique<ledger::NoOverpayment>());
    book.addRule(std::make_unique<ledger::MinimumInstallment>(ledger::Money{1000}));  // $10 minimum

    MainWindow window{book};
    window.show();
    return app.exec();
}

#include "main.moc"
