#include "pizza.h"

static const char* sizeStr(PizzaSize s) {
    switch (s) {
        case PizzaSize::SMALL: return "S";
        case PizzaSize::LARGE: return "L";
        default:               return "M";
    }
}

std::string RawDough::getInfo() const {
    std::string s = "반죽 #" + std::to_string(m_id) + " [" + sizeStr(m_size) + "]";
    if (m_dough == DoughStage::STRETCHED) s += " 펴짐";
    if (m_dough == DoughStage::BAKED)     s += " 구움";
    if (m_sauce)   s += " 소스";
    if (m_cheese)  s += " 치즈";
    if (m_topping) s += " 토핑";
    if (m_cut)     s += " 자름";
    return s;
}

std::string BoxedPizza::getInfo() const {
    return "완성 피자 #" + std::to_string(m_id) + " [" + sizeStr(m_size) + "] 포장완료";
}
