/*
* Copyright 2012 Przemysław Pastuszka
*/
#ifndef PROTOTYPES_BOB_SRC_COLUMNS_LISTCOLUMN_H_
#define PROTOTYPES_BOB_SRC_COLUMNS_LISTCOLUMN_H_

#include <algorithm>
#include <list>
#include <vector>
#include "Column.h"

template <class T>
class ListColumn : public TypedColumn<T> {
    private:
    std::vector<ListField<T> > fields;

    int leftVisitedBound;
    std::vector<bool> visited;

    public:
    Field<T>* getField(unsigned int i) {
        return &fields[i];
    }

    protected:
    void addField(const T& value, int nextFieldId, bool isLastElement) {
        ListField<T> field;
        field.value = value;
        field.nextFieldId = nextFieldId;
        field.isLastElement = isLastElement;
        fields.push_back(field);
    }
    void addList(std::list<T> *ls, int nextFieldId) {
        typename std::list<T>::iterator left = ls-> begin(), right = ls -> begin();
        right++;
        for(;right != ls -> end(); left++, right++){
            addField(*left, fields.size() + 1, false);
        }
        addField(*left, nextFieldId, true);
    }

    void addValueToResult(int valueIndex, std::list<T>& result, bool markVisitedFields) {
        if(markVisitedFields &&
                0 <= valueIndex - leftVisitedBound &&
                valueIndex - leftVisitedBound < visited.size()) {
            visited[valueIndex - leftVisitedBound] = true;
        }
        result.push_back(fields[valueIndex].value);
    }

    public:
    unsigned int getSize() const { return fields.size(); }
    void sort() {
        std::sort(fields.begin(), fields.end());
    }

    void add(void *value, int nextFieldId) {
        std::list<T> *ls = static_cast<std::list<T>*>(value);
        addList(ls, nextFieldId);
    }
    int lowerBound(const T& value) {
        typename std::vector<ListField<T> >::iterator it =
                std::lower_bound(fields.begin(), fields.end(), value);
        return int(it - fields.begin());
    }
    int upperBound(const T& value) {
        typename std::vector<ListField<T> >::iterator it =
                std::upper_bound(fields.begin(), fields.end(), value);
        return int(it - fields.begin()) - 1;
    }
    int fillRowWithValueAndGetNextFieldId(int valueIndex, int columnIndex, Row* row, bool markVisitedFields) {
        std::list<T> result;

        bool hasAnyFieldInRange = false;

        while(fields[valueIndex].isLastElement == false) {
            hasAnyFieldInRange |= this -> range.isInRange(valueIndex);
            addValueToResult(valueIndex, result, markVisitedFields);
            valueIndex = fields[valueIndex].nextFieldId;
        }
        hasAnyFieldInRange |= this -> range.isInRange(valueIndex);
        addValueToResult(valueIndex, result, markVisitedFields);

        if(hasAnyFieldInRange == false) {
            return -1;
        }

        row -> set<std::list<T> >(columnIndex, result);
        return fields[valueIndex].nextFieldId;
    }
    bool isFieldVisitedAt(int index) {
        return visited[index - leftVisitedBound];
    }
    void markFieldsAsUnvisitedInRange(int left, int right) {
        leftVisitedBound = left;
        visited.clear();
        visited.resize(right - left + 1, false);
    }
};

#endif /* PROTOTYPES_BOB_SRC_COLUMNS_LISTCOLUMN_H_ */
