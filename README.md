# LedgerLite
Small invoice ledger: C++20 core (no Qt), Qt 6 Widgets front end, assert-based tests.

Build and test (Homebrew Qt is keg-only, so point CMake at it):
```
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
cmake --build build
./build/ledger_tests
./build/ledger_app
```
