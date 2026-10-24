#include "Table.h"

#include <gtest/gtest.h>

namespace {
struct ThrowOnCopy {
    bool fail = false;

    ThrowOnCopy() = default;
    ThrowOnCopy(const ThrowOnCopy& other) : fail(other.fail) {
        if (fail) {
            throw std::runtime_error("Copy failed");
        }
    }
};
}  // namespace

TEST(AppendRowTest, RollsBackSuccessfulAppendsWhenCopyThrows) {
    Table table;
    table.addColumn<int>("first");
    table.addColumn<ThrowOnCopy>("second");
    table.addColumn<int>("third");

    ThrowOnCopy value;
    table.appendRow(1, value, 3);
    value.fail = true;

    EXPECT_THROW(table.appendRow(4, value, 6), std::runtime_error);
    EXPECT_EQ(table.rowCount(), 1);
    EXPECT_EQ(table.getColumn<int>("first").size(), 1);
    EXPECT_EQ(table.getColumn<int>("first").get(0), 1);
    EXPECT_EQ(table.getColumn<ThrowOnCopy>("second").size(), 1);
    EXPECT_EQ(table.getColumn<int>("third").size(), 1);
    EXPECT_EQ(table.getColumn<int>("third").get(0), 3);

    value.fail = false;
    table.appendRow(7, value, 9);
    EXPECT_EQ(table.rowCount(), 2);
    EXPECT_EQ(table.getColumn<int>("third").get(1), 9);
}

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

TEST(ColumnOrderTest, ColumnsAreStoredInOrder) {
    Table table;

    table.addColumn<int>("first_column");
    table.addColumn<std::string>("second_column");
    table.addColumn<double>("third_column");

    EXPECT_EQ(table.columnCount(), 3);
    EXPECT_TRUE(table.hasColumn("first_column"));
    EXPECT_TRUE(table.hasColumn("second_column"));
    EXPECT_TRUE(table.hasColumn("third_column"));

    auto& firstColumn = table.getColumn<int>("first_column");
    auto& secondColumn = table.getColumn<std::string>("second_column");
    auto& thirdColumn = table.getColumn<double>("third_column");

    firstColumn.append(1);
    secondColumn.append("two");
    thirdColumn.append(3.0);

    EXPECT_EQ(firstColumn.size(), 1);
    EXPECT_EQ(secondColumn.size(), 1);
    EXPECT_EQ(thirdColumn.size(), 1);

    EXPECT_EQ(firstColumn.get(0), 1);
    EXPECT_EQ(secondColumn.get(0), "two");
    EXPECT_EQ(thirdColumn.get(0), 3.0);
}

TEST(ColumnPopBackTest, PopBackReducesSize) {
    Table table;

    table.addColumn<int>("int_column");
    auto& intColumn = table.getColumn<int>("int_column");

    intColumn.append(1);
    intColumn.append(2);
    intColumn.append(3);

    EXPECT_EQ(intColumn.size(), 3);

    intColumn.popBack();
    EXPECT_EQ(intColumn.size(), 2);
    EXPECT_EQ(intColumn.get(0), 1);
    EXPECT_EQ(intColumn.get(1), 2);

    intColumn.popBack();
    EXPECT_EQ(intColumn.size(), 1);
    EXPECT_EQ(intColumn.get(0), 1);

    intColumn.popBack();
    EXPECT_EQ(intColumn.size(), 0);
}
