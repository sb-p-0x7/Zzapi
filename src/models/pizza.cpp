#include "pizza.h"

Pizza::Pizza(int id)
    : id(id), doughState(DoughState::RAW), size(PizzaSize::MEDIUM),
      hasSauce(false), hasCheese(false),
      hasTopping(false), isBaked(false), sliceCount(0), 
      isCut(false), isPackaged(false) {}

int Pizza::getId() const { return id; }
DoughState Pizza::getDoughState() const { return doughState; }
PizzaSize Pizza::getSize() const { return size; }
bool Pizza::getHasSauce() const { return hasSauce; }
bool Pizza::getHasCheese() const { return hasCheese; }
const std::vector<ToppingType>& Pizza::getToppings() const { return toppings; }
bool Pizza::getHasTopping() const { return hasTopping; }
bool Pizza::getIsBaked() const { return isBaked; }
int Pizza::getSliceCount() const { return sliceCount; }
bool Pizza::getIsCut() const { return isCut; }
bool Pizza::getIsPackaged() const { return isPackaged; }

void Pizza::setDoughState(DoughState state) { doughState = state; }
void Pizza::setSize(PizzaSize s) { size = s; }
void Pizza::setSauce(bool v) { hasSauce = v; }
void Pizza::setCheese(bool v) { hasCheese = v; }
void Pizza::addTopping(ToppingType topping) { toppings.push_back(topping); }
void Pizza::setHasTopping(bool v) { hasTopping = v; }
void Pizza::setIsBaked(bool v) { isBaked = v; }
void Pizza::setSliceCount(int count) { sliceCount = count; }
void Pizza::setIsCut(bool v) { isCut = v; }
void Pizza::setPackaged(bool packaged) { isPackaged = packaged; }
