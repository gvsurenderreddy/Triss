#include "Result.h"

Result::Result(std::list<Row*>& rows) {
    this->rows = rows;
    current = rows.begin();
}

Result::~Result() {
    for (std::list<Row*>::iterator it = rows.begin(); it != rows.end(); ++it) {
        delete *it;
    }
}

bool Result::hasNext() {
    return current != rows.end();
}

Row* Result::next() {
    return *current++;
}

std::list<Row*> Result::fetchAll() {
    current = rows.end();
    return rows;
}