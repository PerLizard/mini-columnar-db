
#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Column.h"
#include "IColumn.h"

class Table {
   public:
    template <typename T>
    void addColumn(const std::string& name);

    template <typename T>
    const Column<T>& getColumn(const std::string& name) const;

    std::size_t columnCount() const noexcept;

    bool hasColumn(const std::string& name) const;

    std::size_t rowCount() const noexcept;

    template <typename... Args>
    void appendRow(Args&&... args);

   private:
    std::vector<std::unique_ptr<IColumn>> columns_;
    std::unordered_map<std::string, std::size_t> column_indices_;

    template <typename T>
    void appendValue(std::size_t index, T&& value);

    template <typename T>
    bool matchesType(std::size_t index) const;
};

template <typename T>
void Table::addColumn(const std::string& name) {
    if (column_indices_.contains(name)) {
        throw std::invalid_argument("Column already exists: " + name);
    }

    if (rowCount() != 0) {
        throw std::logic_error("Cannot add columns to a non-empty table");
    }

    const std::size_t index = columns_.size();

    columns_.push_back(std::make_unique<Column<T>>());

    try {
        column_indices_.emplace(name, index);
    } catch (...) {
        columns_.pop_back();
        throw;
    }
}

template <typename T>
const Column<T>& Table::getColumn(const std::string& name) const {
    auto it = column_indices_.find(name);

    if (it == column_indices_.end()) {
        throw std::out_of_range("Column not found: " + name);
    }

    return dynamic_cast<const Column<T>&>(*columns_[it->second]);
}

inline std::size_t Table::columnCount() const noexcept { return columns_.size(); }

inline bool Table::hasColumn(const std::string& name) const {
    return column_indices_.contains(name);
}

inline std::size_t Table::rowCount() const noexcept {
    if (columns_.empty()) {
        return 0;
    }
    return columns_.front()->size();
}

template <typename T>
void Table::appendValue(std::size_t index, T&& value) {
    auto* column = dynamic_cast<Column<std::decay_t<T>>*>(columns_[index].get());
    if (!column) {
        throw std::bad_cast();
    }
    column->append(std::forward<T>(value));
}

template <typename T>
bool Table::matchesType(std::size_t index) const {
    const auto* column = dynamic_cast<const Column<std::decay_t<T>>*>(columns_[index].get());
    return column != nullptr;
}

template <typename... Args>
void Table::appendRow(Args&&... args) {
    if (columns_.empty()) {
        throw std::logic_error("Cannot append row to table without columns");
    }
    if (sizeof...(args) != columns_.size()) {
        throw std::invalid_argument("Number of values does not match number of columns");
    }
    std::size_t index = 0;
    bool allMatch = (matchesType<Args>(index++) && ...);
    if (!allMatch) {
        throw std::invalid_argument("Type of values does not match type of columns");
    }
    std::size_t inserted = 0;
    try {
        ((appendValue(inserted, std::forward<Args>(args)), ++inserted), ...);
    } catch (...) {
        for (std::size_t i = 0; i < inserted; ++i) {
            columns_[i]->popBack();
        }
        throw;
    }
}
