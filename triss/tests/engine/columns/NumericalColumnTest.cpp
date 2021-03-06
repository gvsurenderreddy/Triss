/*
* Copyright 2012 Przemysław Pastuszka
*/
#define TRISS_TEST

#include <gtest/gtest.h>
#include <cstdarg>
#include <cstdlib>
#include <triss/src/engine/columns/ScalarColumn.h>
#include <triss/src/common/Schema.h>
#include <triss/src/engine/Table.h>
#include <triss/src/common/Schema.h>
#include <triss/src/utils/Tools.h>

class NumericalColumnTest : public ::testing::Test {
    public:
    ScalarColumn<double> c;
    std::vector<int> mappings[2];
    TableRow* row;
    std::vector<ColumnDesc> schema;

    virtual void SetUp() {
        c.setColumnId(1);
        c.setGlobalPosition(0);

        schema.clear();
        schema.push_back(ColumnDesc("a", Schema::NUMERICAL));
        schema.push_back(ColumnDesc("b", Schema::NUMERICAL));
        Table table;
        table.setSchema(schema);
        row = table.createTableRow();

        double initialValues[] = {5, 12, 7, 8, 19, 1};
        for(int i = 0; i < 6; ++i) {
            row -> set<double>(1, initialValues[i]);
            c.add(*row, 5 - i);
        }
    }

    virtual void TearDown() {
        delete row;
    }
};

TEST_F(NumericalColumnTest, shouldBeSorted) {
    c.sort();

    double sortedValues[] = {1, 5, 7, 8, 12, 19};
    for(int i = 0; i < 6; ++i) {
        ASSERT_EQ(sortedValues[i], c.getField(i) -> value);
    }
}

TEST_F(NumericalColumnTest, shouldFillRowWithGoodValue) {
    c.createMappingFromCurrentToSortedPositions(mappings[1]);
    c.updateNextFieldIdsUsingMapping(mappings[0], mappings[1], 0);

    ColumnQueryState* state = c.prepareColumnForQuery();
    c.reduceConstraintsToRangeSet(state);
    c.markAsMainQueryColumn(state);
    state -> positionsInResult = Tools::vector<int>(2, /**/ 0, 1);

    ASSERT_EQ(5, c.fillRowWithValueAndGetNextFieldId(1, 1, row, state, schema, true));
    ASSERT_EQ(12, row -> get<double>(1));

    delete state;
}

TEST_F(NumericalColumnTest, shouldCreateValidMapping) {
    c.createMappingFromCurrentToSortedPositions(mappings[1]);

    int validMapping[] = {1, 4, 2, 3, 5, 0};
    for(int i = 0; i < 6; ++i) {
        ASSERT_EQ(validMapping[i], mappings[1][i]);
    }
}

TEST_F(NumericalColumnTest, shouldUpdateNextIdsUsingMapping) {
    int shift = 3;
    c.createMappingFromCurrentToSortedPositions(mappings[1]);
    c.updateNextFieldIdsUsingMapping(mappings[0], mappings[1], shift);

    int validNextFields[] = {0, 5, 3, 2, 4, 1};
    for(int i = 0; i < 6; ++i) {
        ASSERT_EQ(validNextFields[i] + shift, c.getField(i) -> nextFieldId);
    }
}
