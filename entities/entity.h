#ifndef ENTITY_H
#define ENTITY_H

#include <QDebug>
#include <QDate>
#include <chrono>
#include "qtextbrowser.h"
#include <set>
#include <unordered_map>

#define DOLLAR "$"
#define POUND "£"
#define EURO "€"
#define HRYVNA "₴"

class Entity
{
private:
    static const std::unordered_map<QString, bool> m_possibleCur;
    static const std::unordered_map<QString, double> m_dollarCost;
protected:
    int m_id = 0;
public:
    Entity(const int id);
    Entity() = default;
    virtual ~Entity() = default;
    int getId() const;
    void setId(const int id);
    static inline bool isCurrencySupported(const QString& cur){
        try{
            return m_possibleCur.at(cur);
        }catch(const std::out_of_range& e){}
        return false;
    }
    static inline double toDollar(const double amount, const QString& cur){
        const bool currency = isCurrencySupported(cur);
        if(currency)
            return amount / m_dollarCost.at(cur);
        return 0.;
    }
    static inline double fromDollar(const double amount, const QString& cur){
        const bool currency = isCurrencySupported(cur);
        if(currency)
            return amount * m_dollarCost.at(cur);
        return 0.;
    }
    bool operator==(const Entity& other) const{
        return m_id == other.m_id;
    }
    bool operator<(const Entity& other) const{
        return m_id < other.m_id;
    }
};

#endif // ENTITY_H
