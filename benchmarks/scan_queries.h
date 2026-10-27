#pragma once

#include <cstdint>
#include <vector>

#include "Column.h"

struct Trade {
    std::int64_t timestamp;
    double price;
    double volume;
    std::int64_t symbolId;
};

// Implemented in a separate translation unit; the benchmark target disables LTO.
double scanRows(const std::vector<Trade>& rows, double threshold);
double scanColumns(const Column<double>& prices, const Column<double>& volumes, double threshold);
double scanOperators(const Column<double>& prices, const Column<double>& volumes, double threshold);
double scanSumWhere(const Column<double>& prices, const Column<double>& volumes, double threshold);
