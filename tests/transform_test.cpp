#include "Transform.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Aggregate.h"
#include "Filter.h"
#include "Projection.h"
#include "Table.h"

namespace {
template <typename T>
Column<T> makeColumn(std::initializer_list<T> values) {
    Column<T> column;
    for (const auto& value : values) {
        column.append(value);
    }
    return column;
}

template <typename T>
void expectValues(const Column<T>& column, std::initializer_list<T> values) {
    ASSERT_EQ(column.size(), values.size());
    std::size_t i = 0;
    for (const auto& value : values) {
        EXPECT_EQ(column.get(i++), value);
    }
}

constexpr auto multiply = [](const double& price, const int& volume) { return price * volume; };
static_assert(
    std::is_same_v<decltype(transformColumns(std::declval<const Column<double>&>(),
                                             std::declval<const Column<int>&>(), multiply)),
                   Column<double>>);
}  // namespace

TEST(TransformTest, EmptyColumnsReturnEmptyResultWithoutCallingOperation) {
    const Column<double> left;
    const Column<int> right;
    int calls = 0;

    const auto result = transformColumns(left, right, [&calls](double a, int b) {
        ++calls;
        return a * b;
    });

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.size(), 0);
    EXPECT_EQ(calls, 0);
}

TEST(TransformTest, DifferentSizesThrowBeforeCallingOperation) {
    const auto shorter = makeColumn<int>({1});
    const auto longer = makeColumn<int>({2, 3});
    const Column<int> empty;
    int calls = 0;
    auto operation = [&calls](int a, int b) {
        ++calls;
        return a + b;
    };

    EXPECT_THROW(transformColumns(shorter, longer, operation), std::invalid_argument);
    EXPECT_THROW(transformColumns(longer, shorter, operation), std::invalid_argument);
    EXPECT_THROW(transformColumns(empty, shorter, operation), std::invalid_argument);
    EXPECT_THROW(transformColumns(shorter, empty, operation), std::invalid_argument);
    EXPECT_EQ(calls, 0);
}

TEST(TransformTest, MultipliesDoubleByIntAndPreservesInputs) {
    const auto prices = makeColumn<double>({1.5, -2.25, 0.1});
    const auto volumes = makeColumn<int>({4, 2, 3});

    const auto result = transformColumns(prices, volumes, multiply);

    ASSERT_EQ(result.size(), 3);
    EXPECT_NEAR(result.get(0), 6.0, 1e-12);
    EXPECT_NEAR(result.get(1), -4.5, 1e-12);
    EXPECT_NEAR(result.get(2), 0.3, 1e-12);
    expectValues(prices, {1.5, -2.25, 0.1});
    expectValues(volumes, {4, 2, 3});
}

TEST(TransformTest, SupportsAddition) {
    const auto left = makeColumn<int>({1, -2, 3});
    const auto right = makeColumn<int>({4, 5, -6});

    const auto result = transformColumns(left, right, [](int a, int b) { return a + b; });

    expectValues(result, {5, 3, -3});
}

TEST(TransformTest, SupportsLambdaWithCapture) {
    const auto left = makeColumn<int>({1, 2});
    const auto right = makeColumn<int>({3, 4});
    const int offset = 10;

    const auto result =
        transformColumns(left, right, [offset = offset](int a, int b) { return a + b + offset; });

    expectValues(result, {14, 16});
}

TEST(TransformTest, CopiesConstReferenceResultsAndOutlivesSourceColumns) {
    Column<std::string> result;
    {
        const auto left = makeColumn<std::string>({std::string(100, 'a'), "short"});
        const auto right = makeColumn<std::string>({"small", std::string(100, 'b')});
        auto transformed = transformColumns(
            left, right, [](const std::string& a, const std::string& b) -> const std::string& {
                return a.size() > b.size() ? a : b;
            });
        static_assert(std::is_same_v<decltype(transformed), Column<std::string>>);
        result = std::move(transformed);
    }

    expectValues(result, {std::string(100, 'a'), std::string(100, 'b')});
}

TEST(TransformTest, CallsOperationOncePerRowInOrder) {
    const auto left = makeColumn<int>({3, 1, 2});
    const auto right = makeColumn<int>({10, 20, 30});
    std::vector<std::pair<int, int>> calls;

    const auto result = transformColumns(left, right, [&calls](int a, int b) {
        calls.emplace_back(a, b);
        return a - b;
    });

    EXPECT_EQ(calls, (std::vector<std::pair<int, int>>{{3, 10}, {1, 20}, {2, 30}}));
    expectValues(result, {-7, -19, -28});
}

TEST(TransformTest, PropagatesExceptionStopsCallsAndPreservesInputs) {
    auto left = makeColumn<int>({1, 2, 3, 4});
    auto right = makeColumn<int>({10, 20, 30, 40});
    std::vector<std::pair<int, int>> calls;

    try {
        transformColumns(left, right, [&calls](int a, int b) {
            calls.emplace_back(a, b);
            if (a == 2) {
                throw std::runtime_error("operation failed");
            }
            return a + b;
        });
        FAIL() << "Expected operation exception";
    } catch (const std::runtime_error& error) {
        EXPECT_STREQ(error.what(), "operation failed");
    }

    EXPECT_EQ(calls, (std::vector<std::pair<int, int>>{{1, 10}, {2, 20}}));
    expectValues(left, {1, 2, 3, 4});
    expectValues(right, {10, 20, 30, 40});
}

TEST(TransformTest, FullAnalyticalQuery) {
    Table table;
    table.addColumn<std::string>("symbol");
    table.addColumn<double>("price");
    table.addColumn<int>("volume");
    table.appendRow(std::string("A"), 100.0, 10);
    table.appendRow(std::string("B"), 250.0, 20);
    table.appendRow(std::string("A"), 200.0, 30);
    table.appendRow(std::string("B"), 300.0, 40);
    table.appendRow(std::string("C"), 150.0, 50);
    const auto& symbols = table.getColumn<std::string>("symbol");
    const auto& prices = table.getColumn<double>("price");
    const auto& volumes = table.getColumn<int>("volume");

    const auto revenue = transformColumns(prices, volumes, multiply);
    const auto indices = filterIndices(prices, [](double price) { return price > 150.0; });
    const auto selectedSymbols = selectRows(symbols, indices);
    const auto selectedRevenue = selectRows(revenue, indices);
    const auto total = sumSelected(revenue, indices);
    const auto groups = groupSumSelected(symbols, revenue, indices);

    EXPECT_EQ(indices, (std::vector<std::size_t>{1, 2, 3}));
    expectValues(selectedSymbols, {std::string("B"), std::string("A"), std::string("B")});
    ASSERT_EQ(selectedRevenue.size(), 3);
    EXPECT_NEAR(selectedRevenue.get(0), 5000.0, 1e-9);
    EXPECT_NEAR(selectedRevenue.get(1), 6000.0, 1e-9);
    EXPECT_NEAR(selectedRevenue.get(2), 12000.0, 1e-9);
    EXPECT_NEAR(total, 23000.0, 1e-9);
    ASSERT_EQ(groups.size(), 2);
    ASSERT_TRUE(groups.contains("A"));
    ASSERT_TRUE(groups.contains("B"));
    EXPECT_NEAR(groups.at("A"), 6000.0, 1e-9);
    EXPECT_NEAR(groups.at("B"), 17000.0, 1e-9);
    expectValues(prices, {100.0, 250.0, 200.0, 300.0, 150.0});
    expectValues(volumes, {10, 20, 30, 40, 50});
}
