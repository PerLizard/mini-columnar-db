#include "Aggregate.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Filter.h"
#include "Table.h"

namespace {
class SumWhereTest : public ::testing::Test {
   protected:
    void SetUp() override {
        for (int value : {3, 1, 4, 2}) {
            filter.append(value);
        }
        for (int value : {10, -4, 7, -2}) {
            values.append(value);
        }
    }

    Column<int> filter;
    Column<int> values;
};

class AggregateTest : public ::testing::Test {
   protected:
    void SetUp() override {
        for (int value : {10, -4, 7, -2}) {
            column.append(value);
        }
    }

    Column<int> column;
};
}  // namespace

TEST(SumWhereStandaloneTest, EmptyColumnsReturnZeroWithoutCallingPredicate) {
    const Column<int> filter;
    const Column<double> values;
    int calls = 0;

    EXPECT_DOUBLE_EQ(sumWhere(filter, values, [&calls](int) {
        ++calls;
        return true;
    }), 0.0);
    EXPECT_EQ(calls, 0);
}

TEST_F(SumWhereTest, MismatchedSizesThrowWithoutCallingPredicate) {
    values.popBack();
    const Column<int> empty;
    int calls = 0;
    auto predicate = [&calls](int) {
        ++calls;
        return true;
    };

    EXPECT_THROW(sumWhere(filter, values, predicate), std::invalid_argument);
    EXPECT_THROW(sumWhere(values, filter, predicate), std::invalid_argument);
    EXPECT_THROW(sumWhere(empty, values, predicate), std::invalid_argument);
    EXPECT_THROW(sumWhere(filter, empty, predicate), std::invalid_argument);
    EXPECT_EQ(calls, 0);
}

TEST_F(SumWhereTest, SumsAllMatches) {
    EXPECT_EQ(sumWhere(filter, values, [](int value) { return value > 0; }), 11);
}

TEST_F(SumWhereTest, SumsOnlyPartialMatches) {
    EXPECT_EQ(sumWhere(filter, values, [](int value) { return value >= 3; }), 17);
}

TEST_F(SumWhereTest, NoMatchesReturnZero) {
    EXPECT_EQ(sumWhere(filter, values, [](int value) { return value > 4; }), 0);
}

TEST(SumWhereStandaloneTest, SupportsStringFilterAndDoubleValues) {
    Column<std::string> filter;
    Column<double> values;
    for (const auto* symbol : {"A", "B", "A", "C"}) {
        filter.append(symbol);
    }
    for (double value : {0.1, 9.0, 0.2, -5.0}) {
        values.append(value);
    }

    const auto result = sumWhere(filter, values, [](const std::string& symbol) {
        return symbol == "A";
    });

    EXPECT_NEAR(result, 0.3, 1e-12);
}

TEST_F(SumWhereTest, CallsPredicateOncePerRowInOrder) {
    std::vector<int> calls;

    const auto result = sumWhere(filter, values, [&calls](int value) {
        calls.push_back(value);
        return value % 2 == 0;
    });

    EXPECT_EQ(calls, (std::vector<int>{3, 1, 4, 2}));
    EXPECT_EQ(result, 5);
}

TEST_F(SumWhereTest, ExceptionStopsTraversalAndPreservesInputs) {
    std::vector<int> calls;

    try {
        sumWhere(filter, values, [&calls](int value) {
            calls.push_back(value);
            if (value == 4) {
                throw std::runtime_error("predicate failed");
            }
            return true;
        });
        FAIL() << "Expected predicate exception";
    } catch (const std::runtime_error& error) {
        EXPECT_STREQ(error.what(), "predicate failed");
    }

    EXPECT_EQ(calls, (std::vector<int>{3, 1, 4}));
    const std::array<int, 4> originalFilter{3, 1, 4, 2};
    const std::array<int, 4> originalValues{10, -4, 7, -2};
    ASSERT_EQ(filter.size(), originalFilter.size());
    ASSERT_EQ(values.size(), originalValues.size());
    for (std::size_t i = 0; i < originalFilter.size(); ++i) {
        EXPECT_EQ(filter.get(i), originalFilter[i]);
        EXPECT_EQ(values.get(i), originalValues[i]);
    }
}

TEST_F(SumWhereTest, MatchesFilterIndicesThenSumSelected) {
    // Cover no matches, partial matches and all matches with the same inputs.
    for (int threshold : {0, 2, 4}) {
        auto predicate = [threshold](int value) { return value > threshold; };
        const auto indices = filterIndices(filter, predicate);

        EXPECT_EQ(sumWhere(filter, values, predicate), sumSelected(values, indices));
    }
}

TEST_F(AggregateTest, EmptySelectionReturnsZeroForNonEmptyColumn) {
    EXPECT_EQ(sumSelected(column, {}), 0);
}

TEST(AggregateStandaloneTest, EmptyColumnAndSelectionReturnZero) {
    const Column<int> empty;

    EXPECT_EQ(sumSelected(empty, {}), 0);
}

TEST_F(AggregateTest, SumsIntegersIncludingNegativeValues) {
    const std::vector<std::size_t> indices{0, 1, 2, 3};

    EXPECT_EQ(sumSelected(column, indices), 11);
}

TEST_F(AggregateTest, SumsOnlySelectedElements) {
    const std::vector<std::size_t> indices{1, 3};

    EXPECT_EQ(sumSelected(column, indices), -6);
}

TEST_F(AggregateTest, AcceptsIndicesFromArray) {
    const std::array<std::size_t, 2> indices{0, 2};

    EXPECT_EQ(sumSelected(column, indices), 17);
}

TEST_F(AggregateTest, AcceptsIndicesInArbitraryOrder) {
    const std::vector<std::size_t> indices{2, 0, 3};

    EXPECT_EQ(sumSelected(column, indices), 15);
}

TEST_F(AggregateTest, CountsRepeatedIndicesEachTime) {
    const std::vector<std::size_t> indices{1, 0, 1, 1};

    EXPECT_EQ(sumSelected(column, indices), -2);
}

TEST_F(AggregateTest, InvalidIndexThrows) {
    const std::vector<std::size_t> atEnd{column.size()};
    const std::vector<std::size_t> farBeyondEnd{std::numeric_limits<std::size_t>::max()};

    EXPECT_THROW(sumSelected(column, atEnd), std::out_of_range);
    EXPECT_THROW(sumSelected(column, farBeyondEnd), std::out_of_range);
}

TEST_F(AggregateTest, InvalidIndexAfterValidIndicesThrows) {
    std::vector<std::size_t> indices{0, 1, 2, column.size()};
    const auto originalIndices = indices;
    std::vector<int> originalValues;
    for (std::size_t i = 0; i < column.size(); ++i) {
        originalValues.push_back(column.get(i));
    }

    EXPECT_THROW(sumSelected(column, indices), std::out_of_range);

    EXPECT_EQ(indices, originalIndices);
    ASSERT_EQ(column.size(), originalValues.size());
    for (std::size_t i = 0; i < originalValues.size(); ++i) {
        EXPECT_EQ(column.get(i), originalValues[i]);
    }
}

TEST(AggregateStandaloneTest, SumsDoubleValues) {
    Column<double> values;
    for (double value : {0.1, -0.2, 0.3, 1.25}) {
        values.append(value);
    }
    const std::vector<std::size_t> indices{0, 1, 2, 3};

    EXPECT_NEAR(sumSelected(values, indices), 1.45, 1e-12);
}

TEST(AggregateStandaloneTest, SumsVolumeForFilteredPrices) {
    Table table;
    table.addColumn<double>("price");
    table.addColumn<int>("volume");
    table.appendRow(100.0, 10);
    table.appendRow(250.0, 20);
    table.appendRow(150.0, 30);
    table.appendRow(300.0, 40);
    const auto& price = table.getColumn<double>("price");
    const auto& volume = table.getColumn<int>("volume");

    const auto indices = filterIndices(price, [](double value) { return value > 150.0; });

    EXPECT_EQ(indices, (std::vector<std::size_t>{1, 3}));
    EXPECT_EQ(sumSelected(volume, indices), 60);
}

TEST(GroupSumTest, EmptyColumnsAndSelectionReturnEmptyMap) {
    const Column<int> keys;
    const Column<int> values;

    EXPECT_TRUE(groupSumSelected(keys, values, {}).empty());
}

TEST(GroupSumTest, NonEmptyColumnsWithEmptySelectionReturnEmptyMap) {
    Column<int> keys;
    Column<int> values;
    keys.append(1);
    values.append(10);

    EXPECT_TRUE(groupSumSelected(keys, values, {}).empty());
}

TEST(GroupSumTest, SumsSingleGroup) {
    Column<int> keys;
    Column<int> values;
    for (int value : {10, -4, 7}) {
        keys.append(5);
        values.append(value);
    }
    const std::array<std::size_t, 3> indices{0, 1, 2};

    const auto groups = groupSumSelected(keys, values, indices);

    EXPECT_EQ(groups, (std::unordered_map<int, int>{{5, 13}}));
}

TEST(GroupSumTest, CombinesNonAdjacentRowsIntoMultipleGroups) {
    Column<int> keys;
    Column<int> values;
    for (int key : {2, 1, 2, 3, 1}) {
        keys.append(key);
    }
    for (int value : {10, 20, -3, 4, 5}) {
        values.append(value);
    }
    const std::array<std::size_t, 5> indices{0, 1, 2, 3, 4};

    const auto groups = groupSumSelected(keys, values, indices);

    EXPECT_EQ(groups, (std::unordered_map<int, int>{{1, 25}, {2, 7}, {3, 4}}));
}

TEST(GroupSumTest, PartialSelectionCountsRepeatedIndices) {
    Column<int> keys;
    Column<int> values;
    for (int key : {1, 2, 1, 3}) {
        keys.append(key);
    }
    for (int value : {10, 20, -3, 40}) {
        values.append(value);
    }
    const std::array<std::size_t, 4> indices{2, 1, 2, 1};

    const auto groups = groupSumSelected(keys, values, indices);

    EXPECT_EQ(groups, (std::unordered_map<int, int>{{1, -6}, {2, 40}}));
}

TEST(GroupSumTest, SupportsStringKeysAndDoubleValues) {
    Column<std::string> keys;
    Column<double> values;
    for (const auto* key : {"apple", "pear", "apple", "pear"}) {
        keys.append(key);
    }
    for (double value : {0.1, 1.25, 0.2, -0.5}) {
        values.append(value);
    }
    const std::array<std::size_t, 4> indices{0, 1, 2, 3};

    const auto groups = groupSumSelected(keys, values, indices);

    ASSERT_EQ(groups.size(), 2);
    ASSERT_TRUE(groups.contains("apple"));
    ASSERT_TRUE(groups.contains("pear"));
    EXPECT_NEAR(groups.at("apple"), 0.3, 1e-12);
    EXPECT_NEAR(groups.at("pear"), 0.75, 1e-12);
}

TEST(GroupSumTest, MismatchedSizesThrowIncludingForEmptySelection) {
    Column<int> keys;
    Column<int> values;
    keys.append(1);
    keys.append(2);
    values.append(10);
    const std::array<std::size_t, 1> indices{0};

    EXPECT_THROW(groupSumSelected(keys, values, indices), std::invalid_argument);
    EXPECT_THROW(groupSumSelected(keys, values, {}), std::invalid_argument);
    EXPECT_THROW(groupSumSelected(values, keys, indices), std::invalid_argument);
    EXPECT_THROW(groupSumSelected(values, keys, {}), std::invalid_argument);

    const Column<int> empty;
    EXPECT_THROW(groupSumSelected(keys, empty, {}), std::invalid_argument);
    EXPECT_THROW(groupSumSelected(empty, values, {}), std::invalid_argument);
}

TEST(GroupSumTest, InvalidIndexAfterValidIndexThrowsAndPreservesInputs) {
    Column<int> keys;
    Column<int> values;
    keys.append(1);
    keys.append(2);
    values.append(10);
    values.append(20);
    std::array<std::size_t, 2> indices{0, 2};
    const auto originalIndices = indices;

    EXPECT_THROW(groupSumSelected(keys, values, indices), std::out_of_range);

    EXPECT_EQ(indices, originalIndices);
    ASSERT_EQ(keys.size(), 2);
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(keys.get(0), 1);
    EXPECT_EQ(keys.get(1), 2);
    EXPECT_EQ(values.get(0), 10);
    EXPECT_EQ(values.get(1), 20);
}

TEST(GroupSumTest, GroupsVolumeForFilteredPrices) {
    Table table;
    table.addColumn<std::string>("symbol");
    table.addColumn<double>("price");
    table.addColumn<int>("volume");
    table.appendRow(std::string("A"), 100.0, 10);
    table.appendRow(std::string("B"), 250.0, 20);
    table.appendRow(std::string("A"), 200.0, 30);
    table.appendRow(std::string("B"), 300.0, 40);
    table.appendRow(std::string("C"), 150.0, 50);
    const auto& price = table.getColumn<double>("price");
    const auto& volume = table.getColumn<int>("volume");
    const auto& symbol = table.getColumn<std::string>("symbol");

    const auto indices = filterIndices(price, [](double value) { return value > 150.0; });
    const auto groups = groupSumSelected(symbol, volume, indices);

    EXPECT_EQ(indices, (std::vector<std::size_t>{1, 2, 3}));
    EXPECT_EQ(groups, (std::unordered_map<std::string, int>{{"A", 30}, {"B", 60}}));
}
