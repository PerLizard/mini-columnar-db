![C++ CI](https://github.com/PerLizard/mini-columnar-db/actions/workflows/ci.yml/badge.svg)

# Mini Columnar DB

An educational in-memory columnar database written in C++20. Each `Column<T>`
stores values of one type in a `std::vector<T>`. `Table` keeps columns in insertion
order and maps their names to indices through a hash table.
Persistence, SQL, and thread synchronization are not implemented yet.

## Build and test

Requires a C++20 compiler and CMake 3.16 or newer. When tests are enabled,
CMake downloads GoogleTest 1.16.0; the initial download requires internet access.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

For GCC and Clang, enable AddressSanitizer and UndefinedBehaviorSanitizer with:

```sh
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build-sanitize
ctest --test-dir build-sanitize --output-on-failure
```

To build without tests, add `-DBUILD_TESTING=OFF` during configuration.
The `mini_db` executable currently only prints a startup message;
the example below demonstrates the header-only API.

## Example

```cpp
#include "Table.h"

#include <string>

int main() {
    Table table;
    table.addColumn<int>("id");
    table.addColumn<std::string>("symbol");
    table.addColumn<double>("price");

    table.appendRow(1, std::string{"BTC"}, 65000.0);
    table.appendRow(2, std::string{"ETH"}, 3500.0);

    const auto& symbols = table.getColumn<std::string>("symbol");
    const auto& symbol = symbols.get(0); // "BTC"
    (void)symbol;
    return table.rowCount() == 2 ? 0 : 1;
}
```

## Table contracts

### Schema and column access

| Method | Behavior and exceptions |
| --- | --- |
| `addColumn<T>(name)` | Appends an empty column. A duplicate name throws `std::invalid_argument`. If allocation fails, the addition is rolled back and the exception propagates. |
| `getColumn<T>(name)` | Returns a mutable reference to the column. A missing name throws `std::out_of_range`; an incorrect `T` throws `std::bad_cast`. |
| `hasColumn(name)` | Checks whether the name exists. |
| `columnCount()` | Returns the number of columns; `noexcept`. |
| `rowCount()` | Returns the first column's size, or `0` if there are no columns; `noexcept`. Does not check whether other columns have the same size. |

Arguments to `appendRow` follow column insertion order, regardless of name order
in the hash table. A consistent table requires all columns to have equal lengths.
The current API allows callers to violate this invariant:

- `addColumn` is allowed after rows have been inserted, but the new column is empty.
- `getColumn` allows independent calls to `append` and `popBack`.

Create the entire schema before inserting the first row, and use `appendRow`
to insert rows. It does not repair existing differences in column lengths.

### appendRow validation

Validation happens before any insertion:

1. If there are no columns, throws `std::logic_error`, even for `appendRow()`.
2. If the number of values differs from the number of columns, throws `std::invalid_argument`.
3. If any type does not match, throws `std::invalid_argument`.

These errors leave the table untouched. Argument types are compared after
applying `std::decay_t`: references and top-level `const` are removed, and arrays
decay to pointers. Implicit conversions between types are not performed:

```cpp
// For columns int, std::string, double:
table.appendRow(1, std::string{"BTC"}, 65000.0); // OK
table.appendRow(1, "BTC", 65000.0);              // Error: const char*, not std::string
table.appendRow(1, std::string{"BTC"}, 65000);   // Error: int, not double
```

### Exceptions and rollback

After validation, values are appended from left to right. The successful insertion
counter advances only after `appendValue` returns. If an insertion throws,
the last value is removed from each previously updated column, and the original
exception is rethrown.

For supported copyable types, this restores the table's sizes and logical values
to their state before the call. For example, if the third insertion fails to
allocate memory, the first two insertions are undone. Vector capacities may
remain increased; rollback does not restore references that were invalidated.

Rollback covers the table's data. It does not undo side effects of user-defined
copy constructors, argument evaluation before the call, or changes to external
objects. Do not pass references to elements of this same table if an earlier
insertion could invalidate them by reallocating storage.

Restoring the table and restoring the supplied arguments are separate concerns:

```cpp
std::string symbol = "BTC";
table.appendRow(1, std::move(symbol), 65000.0);
```

**In the current implementation**, `Column<T>::append(const T&)` copies the value.
`std::forward` in `appendValue` preserves the argument's value category, but there
is no `append(T&&)` overload, so `symbol` is not moved from here. `std::move`
itself only enables moving; it does not perform a move.

**If move support is added**, a successful insertion into the second column may
consume the contents of `symbol`. If the third insertion then throws, rollback
restores the table, but `symbol` may remain moved from. A moved-from `std::string`
is valid but has an unspecified value; do not assume it is empty. Restoring
original arguments is outside the rollback contract. To preserve an argument,
pass it as an lvalue and use copying.

## Column and element type contracts

| Method | Behavior |
| --- | --- |
| `append(const T&)` | Copies a value to the end; copying or allocation may throw. |
| `get(index) const` | Returns `const T&`; throws `std::out_of_range` if `index >= size()`. |
| `size()`, `empty()` | Query the size; `noexcept`. |
| `popBack()` | Removes the last value; does nothing if the column is empty; `noexcept`. |

Use ordinary value types without references or top-level `const`, suitable for
storage in `std::vector`. The current `append` requires copying; insertion of
move-only types such as `std::unique_ptr<int>` is not supported. Element
destructors must not throw: rollback removes values through `noexcept popBack()`.

`Column<bool>` is currently unsupported by `get`: the `std::vector<bool>`
specialization does not return an ordinary `const bool&` reference.

A reference returned by `getColumn` remains valid when other columns are added:
columns themselves live in separate objects owned by `unique_ptr`.
References returned by `Column::get` follow `std::vector` rules: reallocation
during `append` invalidates all element references, and `popBack` invalidates
the reference to the removed element. Destroying the table invalidates references
to its columns and elements. Concurrent reads and writes require external
synchronization.

## Formatting

Formatting rules are defined in `.clang-format`. The VS Code workspace settings
enable C++ formatting on save through the clangd extension.
To apply formatting manually:

```sh
git ls-files -z -- '*.cpp' '*.h' | xargs -0 -r clang-format -i
```

CI checks the build, tests, and formatting. The `clang-format --dry-run --Werror`
check reports formatting violations without changing files.
