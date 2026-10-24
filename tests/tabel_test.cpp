#include <gtest/gtest.h>

#include "Table.h"

TEST(TableTest, InitiallyEmpty) {
    Table table;

    EXPECT_EQ(table.columnCount(), 0);
}

TEST(AddColumnTest, AddAndRetrieveColumn) {
    Table table;

    table.addColumn<int>("int_column");
    table.addColumn<std::string>("string_column");

    EXPECT_EQ(table.columnCount(), 2);
    EXPECT_TRUE(table.hasColumn("int_column"));
    EXPECT_TRUE(table.hasColumn("string_column"));

    auto& intColumn = table.getColumn<int>("int_column");
    auto& stringColumn = table.getColumn<std::string>("string_column");

    intColumn.append(42);
    stringColumn.append("Hello");

    EXPECT_EQ(intColumn.size(), 1);
    EXPECT_EQ(stringColumn.size(), 1);
    EXPECT_EQ(intColumn.get(0), 42);
    EXPECT_EQ(stringColumn.get(0), "Hello");
}

TEST(DuplicateColumnTest, AddDuplicateColumnThrows) {
    Table table;

    table.addColumn<int>("int_column");

    EXPECT_THROW(table.addColumn<int>("int_column"), std::invalid_argument);
}

TEST(MissingColumnTest, GetMissingColumnThrows) {
    Table table;

    EXPECT_THROW(table.getColumn<int>("missing_column"), std::out_of_range);
}

TEST(WrongTypeColumnTest, GetColumnWithWrongTypeThrows) {
    Table table;

    table.addColumn<int>("int_column");

    EXPECT_THROW(table.getColumn<std::string>("int_column"), std::bad_cast);
}
