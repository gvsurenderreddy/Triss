/*
* Copyright 2012 Michał Rychlik
*/
#ifndef PROTOTYPES_ALICE_SRC_COLUMNS_FIELD_H_
#define PROTOTYPES_ALICE_SRC_COLUMNS_FIELD_H_

#include <list>
#include <prototypes/common/src/Constraint.h>

namespace Alice {

    class Field {
    protected:
        Field(int id) : id(id) {};
    public:
        int id;
    };

    template <class T>
    class TypedField : public Field {
        private:
            T value;
        public:
        
        TypedField(T value, int id) : Field(id), value(value){}

        bool operator<(const TypedField<T>& other) const {
            return value < other.value;
        }
        bool operator<(const T& other) const {
            return value < other;
        }
        
        template <class C> friend bool operator<(const C&, const TypedField<C>&);
        template <class C> friend bool operator<(const C&, const TypedConstraint<C>&);
        template <class C> friend bool operator<(const TypedConstraint<C>&, const C&);
    };


    template <class C>
    bool operator<(const C& left, const TypedField<C>& other) {
        return left < other.value;
    };

    template <class C>
    bool operator<(const C& left, const TypedConstraint<C>& other) {
        return left < other.getConstraintValue();
    };

    template <class C> 
    bool operator<(const TypedConstraint<C>& other, const C& left) {
        return other.getConstraintValue() < left;  
    };


    class ListField {
    public:
        ListField() : id(0) {};
    protected:
        int id;
        ListField(int id): id(id) {};
    };


    template <class T>
    class TypedListField : public ListField {
        private:
            std::list<T> valuesList;
        public:
            TypedListField(std::list<T>& list, int id) : ListField(id), valuesList(list) {};
            std::list< TypedField<T>* > get_fields() {
                std::set<T> uniqueValues;
                for (typename std::list<T>::iterator it = valuesList.begin(); it != valuesList.end(); ++it) {
                    uniqueValues.insert(*it);
                }
                std::list< TypedField<T>* > result;
                for (typename std::set<T>::iterator it = uniqueValues.begin(); it != uniqueValues.end(); ++it) {
                    result.push_back(new TypedField<T>(*it, id));
                }
                return result;
            }
    };

}
#endif /* PROTOTYPES_ALICE_SRC_COLUMNS_FIELD_H_ */
