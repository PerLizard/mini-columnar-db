#include "Filter.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

TEST(FilterTest, EmptyColumnDoesNotCallPredicate) {
    const Column<int> column;
    std::size_t calls = 0;

    const auto indices = filterIndices(column, [&calls](int) {
        ++calls;
        return true;
    });

    EXPECT_TRUE(indices.empty());
    EXPECT_EQ(calls, 0);
}

TEST(FilterTest, NoMatchingElements) {
    Column<int> column;
    column.append(1);
    column.append(3);
    column.append(5);

    EXPECT_TRUE(filterIndices(column, [](int value) { return value % 2 == 0; }).empty());
}

TEST(FilterTest, AllElementsMatch) {
    Column<int> column;
    column.append(2);
    column.append(4);
    column.append(6);

    const auto indices = filterIndices(column, [](int value) { return value % 2 == 0; });

    EXPECT_EQ(indices, (std::vector<std::size_t>{0, 1, 2}));
}

TEST(FilterTest, PartialMatchesReturnExactIndicesInOrder) {
    Column<int> column;
    for (int value : {9, 4, 7, 2, 4, 1}) {
        column.append(value);
    }

    const auto indices = filterIndices(column, [](int value) { return value % 2 == 0; });

    EXPECT_EQ(indices, (std::vector<std::size_t>{1, 3, 4}));
}

TEST(FilterTest, StringPredicateAcceptsConstReference) {
    Column<std::string> column;
    column.append("pear");
    column.append("a");
    column.append("apple");
    column.append("");

    const auto indices =
        filterIndices(column, [](const std::string& value) { return value.size() >= 4; });

    EXPECT_EQ(indices, (std::vector<std::size_t>{0, 2}));
}

TEST(FilterTest, LambdaCapturesThreshold) {
    Column<int> column;
    for (int value : {3, 10, 7, 11}) {
        column.append(value);
    }
    int threshold = 7;

    const auto indices =
        filterIndices(column, [threshold](int value) { return value > threshold; });

    EXPECT_EQ(indices, (std::vector<std::size_t>{1, 3}));
}

TEST(FilterTest, CallsPredicateOncePerElement) {
    Column<int> column;
    for (int value : {1, 2, 3, 4}) {
        column.append(value);
    }
    std::size_t calls = 0;

    const auto indices = filterIndices(column, [&calls](int value) {
        ++calls;
        return value % 2 == 0;
    });

    EXPECT_EQ(calls, column.size());
    EXPECT_EQ(indices, (std::vector<std::size_t>{1, 3}));
}

TEST(FilterTest, PropagatesPredicateExceptionAndPreservesValues) {
    Column<int> column;
    const std::vector<int> values{4, 8, 12, 16};
    for (int value : values) {
        column.append(value);
    }
    std::size_t calls = 0;

    try {
        filterIndices(column, [&calls](int value) {
            ++calls;
            if (value == 12) {
                throw std::runtime_error("predicate failed");
            }
            return true;
        });
        FAIL() << "Expected predicate exception";
    } catch (const std::runtime_error& error) {
        EXPECT_STREQ(error.what(), "predicate failed");
    }

    EXPECT_EQ(calls, 3);
    ASSERT_EQ(column.size(), values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        EXPECT_EQ(column.get(i), values[i]);
    }
}

TEST(FilterTest, PreservesOriginalValues) {
    Column<std::string> column;
    const std::vector<std::string> values{"pear", "", "apple", "pear"};
    for (const auto& value : values) {
        column.append(value);
    }

    const auto indices =
        filterIndices(column, [](const std::string& value) { return value == "pear"; });

    EXPECT_EQ(indices, (std::vector<std::size_t>{0, 3}));
    ASSERT_EQ(column.size(), values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        EXPECT_EQ(column.get(i), values[i]);
    }
}
