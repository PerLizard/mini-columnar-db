
#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>

#include "Column.h"
#include "IColumn.h"

class Table {
   public:
    template <typename T>
    void addColumn(const std::string& name);

    template <typename T>
    Column<T>& getColumn(const std::string& name);

    std::size_t columnCount() const noexcept;

    bool hasColumn(const std::string& name) const;

   private:
    std::unordered_map<std::string, std::unique_ptr<IColumn>> columns_;
};

template <typename T>
void Table::addColumn(const std::string& name) {
    if (hasColumn(name)) {
        throw std::invalid_argument("Column already exists: " + name);
    }
    columns_[name] = std::make_unique<Column<T>>();
}

template <typename T>
Column<T>& Table::getColumn(const std::string& name) {
    auto it = columns_.find(name);
    if (it == columns_.end()) {
        throw std::out_of_range("Column not found: " + name);
    }
    return dynamic_cast<Column<T>&>(*it->second);
}

inline std::size_t Table::columnCount() const noexcept { return columns_.size(); }

inline bool Table::hasColumn(const std::string& name) const {
    return columns_.find(name) != columns_.end();
}