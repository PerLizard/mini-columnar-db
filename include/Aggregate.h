#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#include "Column.h"

template <typename T>
    requires((std::is_integral_v<T> && !std::is_same_v<T, bool>) || std::is_floating_point_v<T>)
T sumSelected(const Column<T>& column, std::span<const std::size_t> indices) {
    T accumulator{};
    for (std::size_t i : indices) {
        accumulator += column.get(i);
    }
    return accumulator;
}

template <typename Key, typename Value>
    requires((std::is_integral_v<Value> && !std::is_same_v<Value, bool>) ||
             std::is_floating_point_v<Value>)
std::unordered_map<Key, Value> groupSumSelected(const Column<Key>& keys,
                                                const Column<Value>& values,
                                                std::span<const std::size_t> indices) {
    if (keys.size() != values.size()) {
        throw std::invalid_argument("Columns have different sizes");
    }
    std::unordered_map<Key, Value> groups;
    for (std::size_t i : indices) {
        groups[keys.get(i)] += values.get(i);
    }
    return groups;
}

template <typename FilterT, typename ValueT, typename Predicate>
    requires((std::is_integral_v<ValueT> && !std::is_same_v<ValueT, bool>) ||
             std::is_floating_point_v<ValueT>)
ValueT sumWhere(const Column<FilterT>& filterColumn, const Column<ValueT>& valueColumn,
                Predicate predicate) {
    if (filterColumn.size() != valueColumn.size()) {
        throw std::invalid_argument("Columns have different sizes");
    }
    ValueT result{};
    for (std::size_t i = 0; i < filterColumn.size(); ++i) {
        if (predicate(filterColumn.get(i))) {
            result += valueColumn.get(i);
        }
    }
    return result;
}