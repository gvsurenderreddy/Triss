/*
* Copyright 2012 Przemysław Pastuszka
*/
#include <cstdlib>
#include <string>
#include "Table.h"
#include "columns/ScalarColumn.h"
#include "columns/ListColumn.h"
#include <triss/src/common/ValueRange.h>
#include <triss/src/common/Row.h>
#include <triss/src/utils/TrissException.h>

void Table::prepareColumns() {
    columns.reserve(schema.size());
    for(unsigned int i = 0; i < schema.size();++i){
        columns[i] = generateColumn(schema[i].type);
        columns[i] -> setColumnId(i);
    }
}

Column* Table :: generateColumn(Schema::DataType type) {
    switch(type) {
        case Schema::STRING:
            return new ScalarColumn<std::string>();
        case Schema::NUMERICAL:
            return new ScalarColumn<double>();
        case Schema::NUMERICAL_LIST:
            return new ListColumn<double>();
        case Schema::STRING_LIST:
            return new ListColumn<std::string>();
    }
    throw TrissException() << "Unknown data type in given schema: " << type;
}

void Table::addRow(TableRow& row) {
    int initialFirstColumnSize = columns[0] -> getSize();
    for(unsigned int i = 0; i < schema.size() - 1; ++i) {
        columns[i] -> add(row, columns[i + 1] -> getSize());
    }
    columns[schema.size() - 1] -> add(row, initialFirstColumnSize);
}

void Table::prepareStructure() {
    prepareCrossColumnPointers();
    makePointersSkipNullValues();
    removeNullsFromColumns();
    sortColumns();
}

void Table::prepareCrossColumnPointers() {
    std::vector<int> mappings[2];
    int totalNumberOfFields = columns[schema.size() - 1] -> getSize();

    columns[0] -> createMappingFromCurrentToSortedPositions(mappings[1]);
    unsigned int i;
    for(i = 0; i < schema.size() - 1; ++i) {
        columns[i + 1] -> createMappingFromCurrentToSortedPositions(mappings[i % 2]);

        columns[i] -> setGlobalPosition(totalNumberOfFields);
        totalNumberOfFields += columns[i] -> getSize();

        columns[i] -> updateNextFieldIdsUsingMapping(mappings[(i + 1) % 2], mappings[i % 2], totalNumberOfFields);
    }

    columns[0] -> createMappingFromCurrentToSortedPositions(mappings[i % 2]);
    columns[schema.size() - 1] -> setGlobalPosition(totalNumberOfFields);
    columns[schema.size() - 1] -> updateNextFieldIdsUsingMapping(mappings[(i + 1) % 2], mappings[i % 2], columns[schema.size() - 1] -> getSize());
}

void Table::makePointersSkipNullValues() {
    for(unsigned int i = 0; i < schema.size(); ++i) {
        int globalPosition = columns[i] -> getGlobalPosition();
        for(unsigned int j = 0; j < columns[i] -> getSize(); ++j) {
            int nextFieldId = columns[i] -> getNextFieldIdAt(j + globalPosition);
            if(nextFieldId >= columns[i] -> getSize() && columns[i] -> hasNullValueAt(j + globalPosition) == false) {
                for(unsigned int z = 1; z <= schema.size(); ++z) {
                    int nextColumnId = (i + z) % schema.size();
                    if(columns[nextColumnId] -> hasNullValueAt(nextFieldId)) {
                        nextFieldId = columns[nextColumnId] -> getNextFieldIdAt(nextFieldId);
                    }
                    else {
                        columns[i] -> setNextFieldIdAt(j + globalPosition, nextFieldId);
                        break;
                    }
                }
            }
        }
    }
}

void Table::removeNullsFromColumns() {
    for(unsigned int i = 0; i < schema.size(); ++i) {
        columns[i] -> removeNullsFromColumn();
    }
}

void Table::sortColumns() {
    for(unsigned int i = 0; i < schema.size(); ++i) {
        columns[i] -> sort();
    }
}

Result* Table::select(const Query& q) const {
    if(q.getLimit() == 0) {
        throw TrissException() << "Limit cannot be set to 0";
    }

    std::vector<ColumnQueryState*> columnStates;

    prepareColumnsForQuery(columnStates);
    applyConstraintsToColumns(q, columnStates);
    MainColumnInfo mainColumnInfo = chooseMainColumn(columnStates);

    Result* results = gatherResults(q, columnStates, mainColumnInfo);

    for(unsigned int i = 0; i < this -> schema.size(); ++i) {
        delete columnStates[i];
    }

    return results;
}

void Table::setupResultSchema(std::vector<ColumnDesc>& resultSchema, const Query& q) const {
    std::list<unsigned int> selectedColumns = q.getSelectedColumns();
    for(std::list<unsigned int>::iterator it = selectedColumns.begin();
            it != selectedColumns.end(); it++) {
        unsigned int columnId = *it;
        validateColumnId(columnId);
        resultSchema.push_back(schema[columnId]);
    }
}

void Table::setupPositionsInResultForEachColumn(std::vector<ColumnQueryState*>& columnState, const Query& q) const {
    std::list<unsigned int> selectedColumns = q.getSelectedColumns();
    int i = 0;
    for(std::list<unsigned int>::iterator it = selectedColumns.begin();
            it != selectedColumns.end(); it++) {
        unsigned int columnId = *it;
        validateColumnId(columnId);
        columnState[columnId] -> positionsInResult.push_back(i++);
    }
}

Result* Table::gatherResults(const Query& q, std::vector<ColumnQueryState*>& columnStates, MainColumnInfo& info) const {
    std::list<Row*>* results = new std::list<Row*>();
    unsigned int limit = q.getLimit();

    std::vector<ColumnDesc> resultSchema;
    setupResultSchema(resultSchema, q);
    setupPositionsInResultForEachColumn(columnStates, q);

    for(int k = 0; k < info.mainColumnRange.ranges.size(); ++k) {
        IndexRange& currentRange = info.mainColumnRange.ranges[k];
        Row* row = new Row(resultSchema.size());

        for(int i = 0; i < currentRange.length() && results -> size() < limit; ++i) {
            if(retrieveRowBeginningWith(currentRange.left + i, row, columnStates, info, false, resultSchema)) {
                retrieveRowBeginningWith(currentRange.left + i, row, columnStates, info, true, resultSchema);
                results -> push_back(row);
                row = new Row(resultSchema.size());
            }
        }
        delete row;
    }
    return new Result(resultSchema, results);
}

bool Table::retrieveRowBeginningWith(int startPoint, Row* row, std::vector<ColumnQueryState*>& columnStates, MainColumnInfo& info, bool fill, std::vector<ColumnDesc>& resultSchema) const {
    int nextFieldId = startPoint + columns[info.mainColumnId] -> getGlobalPosition();
    unsigned int i;
    for(i = 0; i <= schema.size() && nextFieldId >= 0; ++i) {
        int nextColumnId = (info.mainColumnId + i) % schema.size();
        int relativeFieldId = nextFieldId - columns[nextColumnId] -> getGlobalPosition();
        if(0 <= relativeFieldId && relativeFieldId < columns[nextColumnId] -> getSize()) {
            nextFieldId = columns[nextColumnId] -> fillRowWithValueAndGetNextFieldId(relativeFieldId, startPoint, row, columnStates[nextColumnId], resultSchema, fill);
        }
        else {
            row -> setNull(nextColumnId, schema);
        }
    }
    return nextFieldId >= 0;
}

void Table::prepareColumnsForQuery(std::vector<ColumnQueryState*>& columnStates) const {
    for(unsigned int i = 0; i < schema.size(); ++i) {
        columnStates.push_back(columns[i] -> prepareColumnForQuery());
    }
}

void Table::applyConstraintsToColumns(const Query& q, std::vector<ColumnQueryState*>& columnStates) const {
    std::list<Constraint*> constraints = q.getConstraints();
    for(std::list<Constraint*>::iterator it = constraints.begin(); it != constraints.end(); it++) {
        Constraint* c = *it;
        unsigned int columnId = c -> getAffectedColumn();
        validateColumnId(columnId);

        try {
          columns[columnId] -> addConstraint(c, columnStates[columnId]);
        }
        catch(TrissException& ex) {
            throw TrissException() << "Column " << columnId << ": " << ex.what();
        }
    }
}

Table::MainColumnInfo Table::chooseMainColumn(std::vector<ColumnQueryState*>& columnStates) const {
    MainColumnInfo info;
    info.mainColumnRange = columns[0] -> reduceConstraintsToRangeSet(columnStates[0]);
    info.mainColumnId = 0;
    for(unsigned int i = 1; i < schema.size(); ++i) {
        IndexRangeSet candidateColumnRange = columns[i] -> reduceConstraintsToRangeSet(columnStates[i]);
        if((candidateColumnRange.length() < info.mainColumnRange.length()
                || columnStates[info.mainColumnId] -> hasAnyConstraint() == false)
                && columnStates[i] -> hasAnyConstraint()) {
            info.mainColumnRange = candidateColumnRange;
            info.mainColumnId = i;
        }
    }
    columns[info.mainColumnId] -> markAsMainQueryColumn(columnStates[info.mainColumnId]);
    return info;
}

void Table::deleteColumns() {
    for(unsigned int i = 0; i < schema.size(); ++i) {
        delete columns[i];
    }
    columns.clear();
    schema.clear();
}

void Table::validateColumnId(unsigned int id) const {
    if(id >= schema.size()) {
        throw TrissException() << "Column id should be in [0, "
                << schema.size() << ") range. Id given: " << id;
    }
}

/*** serialization ***/
void Table::serialize() {}
void Table::deserialize(std::string file) {}
