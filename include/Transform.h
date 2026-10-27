#pragma once

#include <stdexcept>
#include <type_traits>

#include "Column.h"

template <typename Left, typename Right, typename BinaryOp>
auto transformColumns(const Column<Left>& left, const Column<Right>& right, BinaryOp operation) {
    if (left.size() != right.size()) {
        throw std::invalid_argument("Columns have different sizes");
    }
    using ReturnType =
        std::remove_cvref_t<std::invoke_result_t<BinaryOp&, const Left&, const Right&>>;

    Column<ReturnType> result;

    for (std::size_t i = 0; i < left.size(); ++i) {
        result.append(operation(left.get(i), right.get(i)));
    }
    return result;
}