#include "pizza.h"

static const char* sizeStr(PizzaSize s) {
    switch (s) {
        case PizzaSize::SMALL: return "S";
        case PizzaSize::LARGE: return "L";
        default:               return "M";
    }
}

std::string RawDough::getInfo() const {
    std::string s = "Dough #" + std::to_string(m_id) + " [" + sizeStr(m_size) + "]";
    if (m_dough == DoughStage::STRETCHED) s += " stretched";
    if (m_dough == DoughStage::BAKED)     s += " baked";
    if (m_sauce)   s += " sauce";
    if (m_cheese)  s += " cheese";
    if (m_topping) s += " topping";
    if (m_cut)     s += " cut";
    return s;
}

std::string BoxedPizza::getInfo() const {
    return "Pizza #" + std::to_string(m_id) + " [" + sizeStr(m_size) + "] packaged";
}
