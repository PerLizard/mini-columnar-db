#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

#include "Table.h"
#include "scan_queries.h"

namespace {
constexpr std::array<std::size_t, 3> rowCounts{100'000, 1'000'000, 10'000'000};
constexpr std::size_t warmups = 3;
constexpr std::size_t repetitions = 24;
constexpr unsigned seed = 42;
constexpr std::array<const char*, 4> names{
    "A row scan", "B column scan", "C operators", "D sumWhere"};
// Generate all 24 permutations: each variant occupies each position six times.
constexpr auto orders = [] {
    std::array<std::array<std::size_t, 4>, repetitions> result{};
    std::array<std::size_t, 4> order{0, 1, 2, 3};
    std::size_t i = 0;
    do {
        result[i++] = order;
    } while (std::next_permutation(order.begin(), order.end()));
    return result;
}();

// Linear interpolation at p * (n - 1), including the median at p = 0.5.
double percentile(const std::vector<double>& sorted, double p) {
    const double position = p * static_cast<double>(sorted.size() - 1);
    const auto lower = static_cast<std::size_t>(position);
    const auto upper = std::min(lower + 1, sorted.size() - 1);
    return sorted[lower] + (sorted[upper] - sorted[lower]) * (position - lower);
}

void benchmark(const std::vector<Trade>& rows, const Column<double>& prices,
               const Column<double>& volumes, double threshold) {
    std::size_t matched = 0;
    double expected = 0.0;
    for (const auto& row : rows) {
        if (row.price > threshold) {
            ++matched;
            expected += row.volume;
        }
    }

    std::array<std::vector<double>, names.size()> times;
    std::array<double, names.size()> checksums{};
    auto query = [&](std::size_t variant) {
        switch (variant) {
            case 0:
                return scanRows(rows, threshold);
            case 1:
                return scanColumns(prices, volumes, threshold);
            case 2:
                return scanOperators(prices, volumes, threshold);
            default:
                return scanSumWhere(prices, volumes, threshold);
        }
    };
    auto consume = [&](std::size_t variant, double sum) {
        if (!std::isfinite(sum) ||
            std::abs(sum - expected) > 1e-9 * std::max(1.0, std::abs(expected))) {
            throw std::runtime_error("Query sums disagree");
        }
        checksums[variant] += sum;
    };

    for (std::size_t run = 0; run < warmups; ++run) {
        for (auto variant : orders[run % orders.size()]) {
            consume(variant, query(variant));
        }
    }
    for (std::size_t run = 0; run < repetitions; ++run) {
        for (auto variant : orders[run % orders.size()]) {
            const auto start = std::chrono::steady_clock::now();
            const double sum = query(variant);
            const auto end = std::chrono::steady_clock::now();
            // Validate and consume EVERY result outside the timed interval.
            consume(variant, sum);
            times[variant].push_back(
                std::chrono::duration<double, std::milli>(end - start).count());
        }
    }

    std::cout << "\nprice > " << threshold
              << ", selectivity=" << 100.0 * static_cast<double>(matched) / rows.size() << "%\n";
    for (std::size_t variant = 0; variant < names.size(); ++variant) {
        auto& samples = times[variant];
        std::sort(samples.begin(), samples.end());
        const double milliseconds = percentile(samples, 0.5);
        std::cout << names[variant] << ": rows=" << rows.size() << ", matched=" << matched
                  << ", p25_ms=" << percentile(samples, 0.25)
                  << ", median_ms=" << milliseconds
                  << ", p75_ms=" << percentile(samples, 0.75) << ", million_rows/s="
                  << static_cast<double>(rows.size()) / (milliseconds * 1000.0)
                  << ", expected_sum=" << std::setprecision(17) << expected
                  << ", all_runs_checksum=" << checksums[variant] << std::setprecision(6) << '\n';
    }
}
}  // namespace

void runSize(std::size_t rowCount) {
    std::vector<Trade> rows;
    Table table;
    table.addColumn<std::int64_t>("timestamp");
    table.addColumn<double>("price");
    table.addColumn<double>("volume");
    table.addColumn<std::int64_t>("symbolId");
    std::mt19937 generator(seed);
    std::uniform_real_distribution<double> price(0.0, 100000.0);
    std::uniform_real_distribution<double> volume(0.0, 100.0);
    std::uniform_int_distribution<std::int64_t> symbol(0, 999);
    for (std::size_t i = 0; i < rowCount; ++i) {
        const Trade trade{static_cast<std::int64_t>(i), price(generator), volume(generator),
                          symbol(generator)};
        rows.push_back(trade);
        table.appendRow(trade.timestamp, trade.price, trade.volume, trade.symbolId);
    }
    const auto& prices = table.getColumn<double>("price");
    const auto& volumes = table.getColumn<double>("volume");
    // Verify identical representations before timing, including unused fields.
    const auto& timestamps = table.getColumn<std::int64_t>("timestamp");
    const auto& symbols = table.getColumn<std::int64_t>("symbolId");
    if (timestamps.size() != rows.size() || prices.size() != rows.size() ||
        volumes.size() != rows.size() || symbols.size() != rows.size()) {
        throw std::runtime_error("Input representation sizes differ");
    }
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].price != prices.get(i) || rows[i].volume != volumes.get(i) ||
            rows[i].timestamp != timestamps.get(i) || rows[i].symbolId != symbols.get(i)) {
            throw std::runtime_error("Input representations differ");
        }
    }
    std::cout << "sizeof(Trade)=" << sizeof(Trade) << ", seed=" << seed
              << ", warmups=" << warmups << ", repetitions=" << repetitions << '\n';
    benchmark(rows, prices, volumes, 90000.0);
    benchmark(rows, prices, volumes, 10000.0);
}

int main() {
    try {
        for (auto rowCount : rowCounts) {
            std::cout << "\nDataset rows=" << rowCount << '\n';
            runSize(rowCount);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
