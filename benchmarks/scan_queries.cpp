#include "scan_queries.h"

#include <cstddef>

#include "Aggregate.h"
#include "Filter.h"

double scanSumWhere(const Column<double>& prices, const Column<double>& volumes, double threshold) {
    return sumWhere(prices, volumes, [threshold](double price) { return price > threshold; });
}

double scanRows(const std::vector<Trade>& rows, double threshold) {
    double sum = 0.0;
    for (const auto& row : rows) {
        if (row.price > threshold) {
            sum += row.volume;
        }
    }
    return sum;
}

double scanColumns(const Column<double>& prices, const Column<double>& volumes, double threshold) {
    double sum = 0.0;
    for (std::size_t i = 0; i < prices.size(); ++i) {
        if (prices.get(i) > threshold) {
            sum += volumes.get(i);
        }
    }
    return sum;
}

double scanOperators(const Column<double>& prices, const Column<double>& volumes,
                     double threshold) {
    const auto indices =
        filterIndices(prices, [threshold](double price) { return price > threshold; });
    return sumSelected(volumes, indices);
}
