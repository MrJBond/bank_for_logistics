#include "entity.h"

const std::unordered_map<QString, bool> Entity::m_possibleCur = {{DOLLAR, true}, {HRYVNA, true}, {EURO, true}, {POUND, true}};
const std::unordered_map<QString, double> Entity::m_dollarCost = { // hard-coded
    {DOLLAR, 1.0},
    {HRYVNA, 41.75},
    {EURO, 0.86},
    {POUND, 0.75}
};

Entity::Entity(const int id): m_id(id){
    setId(id);
}

int Entity::getId() const{
    return m_id;
}
void Entity::setId(const int id){
    if (id <= 0) {
        throw std::invalid_argument("The id is invalid!");
    }
    m_id = id;
}

