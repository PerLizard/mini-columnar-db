#include "Projection.h"
#include "Filter.h"
#include "Table.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace {
class ProjectionTest : public ::testing::Test {
   protected:
    void SetUp() override {
        for (int value : {10, 20, 30, 40}) {
            column.append(value);
        }
    }

    Column<int> column;
};
}  // namespace

TEST(ProjectionStandaloneTest, EmptyColumnAndSelectionReturnEmptyColumn) {
    const Column<int> column;

    const auto selected = selectRows(column, {});

    EXPECT_TRUE(selected.empty());
    EXPECT_EQ(selected.size(), 0);
}

TEST_F(ProjectionTest, EmptySelectionReturnsEmptyColumn) {
    const auto selected = selectRows(column, {});

    EXPECT_TRUE(selected.empty());
    EXPECT_EQ(selected.size(), 0);
}

TEST_F(ProjectionTest, SelectsOnlyRequestedValues) {
    const std::array<std::size_t, 2> indices{0, 2};

    const auto selected = selectRows(column, indices);

    ASSERT_EQ(selected.size(), 2);
    EXPECT_EQ(selected.get(0), 10);
    EXPECT_EQ(selected.get(1), 30);
}

TEST_F(ProjectionTest, PreservesArbitraryIndexOrder) {
    const std::array<std::size_t, 3> indices{3, 0, 2};

    const auto selected = selectRows(column, indices);

    ASSERT_EQ(selected.size(), 3);
    EXPECT_EQ(selected.get(0), 40);
    EXPECT_EQ(selected.get(1), 10);
    EXPECT_EQ(selected.get(2), 30);
}

TEST_F(ProjectionTest, RepeatsValuesForRepeatedIndices) {
    const std::array<std::size_t, 4> indices{1, 1, 3, 1};

    const auto selected = selectRows(column, indices);

    ASSERT_EQ(selected.size(), 4);
    EXPECT_EQ(selected.get(0), 20);
    EXPECT_EQ(selected.get(1), 20);
    EXPECT_EQ(selected.get(2), 40);
    EXPECT_EQ(selected.get(3), 20);
}

TEST(ProjectionStandaloneTest, SelectsStringValues) {
    Column<std::string> column;
    column.append("apple");
    column.append("pear");
    column.append("");
    const std::array<std::size_t, 3> indices{2, 0, 1};

    const auto selected = selectRows(column, indices);

    ASSERT_EQ(selected.size(), 3);
    EXPECT_EQ(selected.get(0), "");
    EXPECT_EQ(selected.get(1), "apple");
    EXPECT_EQ(selected.get(2), "pear");
}

TEST_F(ProjectionTest, InvalidIndexAfterValidIndexThrowsAndPreservesInputs) {
    std::array<std::size_t, 2> indices{1, column.size()};
    const auto originalIndices = indices;

    EXPECT_THROW(selectRows(column, indices), std::out_of_range);

    EXPECT_EQ(indices, originalIndices);
    ASSERT_EQ(column.size(), 4);
    EXPECT_EQ(column.get(0), 10);
    EXPECT_EQ(column.get(1), 20);
    EXPECT_EQ(column.get(2), 30);
    EXPECT_EQ(column.get(3), 40);
}

TEST_F(ProjectionTest, SourceChangesDoNotChangeResult) {
    const std::array<std::size_t, 2> indices{0, 3};
    const auto selected = selectRows(column, indices);

    column.popBack();
    column.append(99);
    column.append(50);

    EXPECT_EQ(column.get(3), 99);
    ASSERT_EQ(selected.size(), 2);
    EXPECT_EQ(selected.get(0), 10);
    EXPECT_EQ(selected.get(1), 40);
}

TEST(ProjectionStandaloneTest, ResultSurvivesSourceDestruction) {
    Column<std::string> selected;
    {
        Column<std::string> source;
        source.append(std::string(100, 'a'));
        source.append(std::string(100, 'b'));
        const std::array<std::size_t, 2> indices{1, 0};
        selected = selectRows(source, indices);
    }

    ASSERT_EQ(selected.size(), 2);
    EXPECT_EQ(selected.get(0), std::string(100, 'b'));
    EXPECT_EQ(selected.get(1), std::string(100, 'a'));
}

TEST(ProjectionStandaloneTest, SelectsSymbolsAndVolumesForFilteredPrices) {
    Table table;
    table.addColumn<std::string>("symbol");
    table.addColumn<double>("price");
    table.addColumn<int>("volume");
    table.appendRow(std::string("A"), 100.0, 10);
    table.appendRow(std::string("B"), 250.0, 20);
    table.appendRow(std::string("C"), 150.0, 30);
    table.appendRow(std::string("D"), 300.0, 40);
    const auto& price = table.getColumn<double>("price");
    const auto indices = filterIndices(price, [](double value) { return value > 150.0; });

    const auto symbols = selectRows(table.getColumn<std::string>("symbol"), indices);
    const auto volumes = selectRows(table.getColumn<int>("volume"), indices);

    ASSERT_EQ(symbols.size(), 2);
    ASSERT_EQ(volumes.size(), 2);
    EXPECT_EQ(symbols.get(0), "B");
    EXPECT_EQ(symbols.get(1), "D");
    EXPECT_EQ(volumes.get(0), 20);
    EXPECT_EQ(volumes.get(1), 40);
}
