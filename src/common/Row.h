/*
* Copyright 2012 Przemysław Pastuszka
*/
#ifndef TRISS_SRC_COMMON_ROW_H_
#define TRISS_SRC_COMMON_ROW_H_

#include <cstdlib>
#include <vector>
#include <string>
#include <list>
#include "Schema.h"

class Row {
    private:
    void** values;
    std::vector<Schema::DataType> schema;

    void init(const std::vector<Schema::DataType>& schema);
    void deleteFieldAt(int index);

    public:
    Row(const Schema& schema) { init(schema.schema); }
    Row(const std::vector<Schema::DataType>& schema) { init(schema); }

    ~Row() {
        for(int i = 0; i < schema.size(); ++i) {
            deleteFieldAt(i);
        }
        delete [] values;
    }

    template <class T>
    void set(int i, const T& value) {
        deleteFieldAt(i);
        values[i] = new T(value);
    }

    template <class T>
    T& get(int i) const {
        return *static_cast<T*>(values[i]);
    }

    bool isNull(int i) const {
        return values[i] == NULL;
    }
    
    Row* getRowCopy();
};


#endif /* TRISS_SRC_COMMON_ROW_H_ */
