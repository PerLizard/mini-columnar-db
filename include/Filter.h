#pragma once

#include <vector>

#include "Column.h"

template <typename T, typename Predicate>
std::vector<std::size_t> filterIndices(const Column<T>& column, Predicate predicate) {
    std::vector<std::size_t> indices;
    for (std::size_t i = 0; i < column.size(); ++i) {
        if (predicate(column.get(i))) {
            indices.push_back(i);
        }
    }
    return indices;
}