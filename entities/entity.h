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
protected:
    int m_id = 0;
public:
    static const std::set<QString> m_possibleCur;
    static const std::unordered_map<QString, double> m_dollarCost;

    Entity(int id);
    Entity() = default;
    virtual ~Entity() = default;
    int getId() const;
    void setId(const int id);
};

#endif // ENTITY_H
