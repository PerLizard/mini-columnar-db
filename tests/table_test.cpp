#include "Table.h"

#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

namespace {
static_assert(
    std::is_same_v<decltype(std::declval<Table&>().getColumn<int>("column")), const Column<int>&>);
static_assert(std::is_same_v<decltype(std::declval<const Table&>().getColumn<int>("column")),
                             const Column<int>&>);

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
    EXPECT_EQ(table.rowCount(), 0);
}

TEST(AddColumnTest, AddAndRetrieveColumn) {
    Table table;
    table.addColumn<int>("int_column");
    table.addColumn<std::string>("string_column");
    table.appendRow(42, std::string("Hello"));

    EXPECT_EQ(table.columnCount(), 2);
    EXPECT_EQ(table.rowCount(), 1);
    EXPECT_TRUE(table.hasColumn("int_column"));
    EXPECT_TRUE(table.hasColumn("string_column"));
    const auto& intColumn = table.getColumn<int>("int_column");
    const auto& stringColumn = table.getColumn<std::string>("string_column");
    ASSERT_EQ(intColumn.size(), 1);
    ASSERT_EQ(stringColumn.size(), 1);
    EXPECT_EQ(intColumn.get(0), 42);
    EXPECT_EQ(stringColumn.get(0), "Hello");
}

TEST(AddColumnTest, AddsMultipleColumnsWhileTableIsEmpty) {
    Table table;
    table.addColumn<int>("first");
    EXPECT_EQ(table.rowCount(), 0);
    table.addColumn<std::string>("second");
    EXPECT_EQ(table.rowCount(), 0);
    table.addColumn<double>("third");

    EXPECT_EQ(table.columnCount(), 3);
    EXPECT_EQ(table.rowCount(), 0);
    EXPECT_TRUE(table.hasColumn("first"));
    EXPECT_TRUE(table.hasColumn("second"));
    EXPECT_TRUE(table.hasColumn("third"));
    EXPECT_TRUE(table.getColumn<int>("first").empty());
    EXPECT_TRUE(table.getColumn<std::string>("second").empty());
    EXPECT_TRUE(table.getColumn<double>("third").empty());
}

TEST(ColumnOrderTest, ColumnsAreStoredInOrder) {
    Table table;
    table.addColumn<int>("first_column");
    table.addColumn<std::string>("second_column");
    table.addColumn<double>("third_column");
    table.appendRow(1, std::string("two"), 3.0);

    EXPECT_EQ(table.columnCount(), 3);
    EXPECT_EQ(table.rowCount(), 1);
    const Table& constTable = table;
    const auto& first = constTable.getColumn<int>("first_column");
    const auto& second = constTable.getColumn<std::string>("second_column");
    const auto& third = constTable.getColumn<double>("third_column");
    ASSERT_EQ(first.size(), 1);
    ASSERT_EQ(second.size(), 1);
    ASSERT_EQ(third.size(), 1);
    EXPECT_EQ(first.get(0), 1);
    EXPECT_EQ(second.get(0), "two");
    EXPECT_EQ(third.get(0), 3.0);
}

TEST(AddColumnTest, RejectsNewColumnAfterAppendWithoutChangingSchemaOrData) {
    Table table;
    table.addColumn<int>("id");
    table.addColumn<std::string>("name");
    table.appendRow(1, std::string("first"));

    EXPECT_THROW(table.addColumn<double>("extra"), std::logic_error);

    EXPECT_EQ(table.columnCount(), 2);
    EXPECT_EQ(table.rowCount(), 1);
    EXPECT_TRUE(table.hasColumn("id"));
    EXPECT_TRUE(table.hasColumn("name"));
    EXPECT_FALSE(table.hasColumn("extra"));
    EXPECT_THROW(table.getColumn<double>("extra"), std::out_of_range);
    ASSERT_EQ(table.getColumn<int>("id").size(), 1);
    ASSERT_EQ(table.getColumn<std::string>("name").size(), 1);
    EXPECT_EQ(table.getColumn<int>("id").get(0), 1);
    EXPECT_EQ(table.getColumn<std::string>("name").get(0), "first");

    table.appendRow(2, std::string("second"));
    EXPECT_EQ(table.rowCount(), 2);
    EXPECT_EQ(table.getColumn<int>("id").get(1), 2);
    EXPECT_EQ(table.getColumn<std::string>("name").get(1), "second");
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
