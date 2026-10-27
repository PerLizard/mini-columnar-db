#pragma once

#include <cstddef>
#include <span>

#include "Column.h"

template <typename T>
Column<T> selectRows(const Column<T>& column, std::span<const std::size_t> indices) {
    Column<T> selected;
    for (std::size_t i : indices) {
        selected.append(column.get(i));
    }
    return selected;
}