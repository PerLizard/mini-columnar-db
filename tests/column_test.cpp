#include "Column.h"

#include <gtest/gtest.h>

TEST(ColumnTest, InitiallyEmpty) {
    Column<int> column;

    EXPECT_TRUE(column.empty());
    EXPECT_EQ(column.size(), 0);
}

TEST(ColumnTest, AppendAndGet) {
    Column<int> column;

    column.append(42);
    column.append(7);

    EXPECT_FALSE(column.empty());
    EXPECT_EQ(column.size(), 2);
    EXPECT_EQ(column.get(0), 42);
    EXPECT_EQ(column.get(1), 7);
}

TEST(ColumnTest, GetOutOfBounds) {
    Column<int> column;

    column.append(42);

    EXPECT_THROW(column.get(1), std::out_of_range);
}