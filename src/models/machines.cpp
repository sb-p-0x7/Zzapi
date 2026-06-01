#include "machines.h"

// =============================================================================
// DoughStretcher::process()
// =============================================================================
void DoughStretcher::process() {
  if (!isPoweredOn) return;
  for (Pizza* pizza : pizzasInProcess) {
    if (pizza) {
      pizza->setDoughState(DoughState::STRETCHED);
      pizza->setSize(targetSize);
    }
  }
}

// =============================================================================
// SauceSpreader::process()
// =============================================================================
void SauceSpreader::process() {
  if (!isPoweredOn) return;
  for (Pizza* pizza : pizzasInProcess) {
    if (pizza) {
      pizza->setSauce(true);
    }
  }
}

// =============================================================================
// CheeseSpreader::process()
// =============================================================================
void CheeseSpreader::process() {
  if (!isPoweredOn) return;
  for (Pizza* pizza : pizzasInProcess) {
    if (pizza) {
      pizza->setCheese(true);
    }
  }
}

// =============================================================================
// ToppingApplier::process()
// =============================================================================
void ToppingApplier::process() {
  if (!isPoweredOn) return;
  for (Pizza* pizza : pizzasInProcess) {
    if (pizza) {
      pizza->setHasTopping(true);
    }
  }
}

// =============================================================================
// Oven::process()
// =============================================================================
void Oven::process() {
  if (!isPoweredOn) return;
  for (Pizza* pizza : pizzasInProcess) {
    if (pizza) {
      pizza->setDoughState(DoughState::BAKED);
      pizza->setIsBaked(true);
    }
  }
}

// =============================================================================
// Cutter::process()
// =============================================================================
void Cutter::process() {
  if (!isPoweredOn) return;
  for (Pizza* pizza : pizzasInProcess) {
    if (pizza) {
      pizza->setIsCut(true);
    }
  }
}

// =============================================================================
// PackagingMachine::process()
// =============================================================================
void PackagingMachine::process() {
  if (!isPoweredOn) return;
  for (Pizza* pizza : pizzasInProcess) {
    if (pizza) {
      pizza->setPackaged(true);
    }
  }
}

// =============================================================================
// Machine
// =============================================================================
void Machine::tick() {
  if (isBroken) {
    currentRepairTimer--;
    if (currentRepairTimer <= 0) {
      isBroken = false;
      durability = maxDurability;
    }
  }
}

void Machine::decreaseDurability(float amount) {
  if (isBroken) return;
  durability -= amount;
  if (durability <= 0.0f) {
    durability = 0.0f;
    isBroken = true;
    currentRepairTimer = repairTime;
  }
}

// =============================================================================
// NonConveyorMachine
// =============================================================================
bool NonConveyorMachine::canInsert() const {
  return (int)pizzasInProcess.size() < capacity;
}

bool NonConveyorMachine::insertPizza(Pizza* pizza) {
  if ((int)pizzasInProcess.size() >= capacity) {
    return false;
  }
  if ((int)pizzasInProcess.size() >= capacity * 0.85f) {
    decreaseDurability(1.0f);
  }
  pizzasInProcess.push_back(pizza);
  return true;
}

bool NonConveyorMachine::hasPizzaToEject() const {
  return !pizzasInProcess.empty();
}

Pizza* NonConveyorMachine::ejectPizza() {
  if (pizzasInProcess.empty()) {
    return nullptr;
  }
  Pizza* pizza = pizzasInProcess.front();
  pizzasInProcess.erase(pizzasInProcess.begin());
  return pizza;
}

// =============================================================================
// ConveyorMachine
// =============================================================================
bool ConveyorMachine::canInsert() const {
  return belt[0] == nullptr;
}

bool ConveyorMachine::insertPizza(Pizza* pizza) {
  if (belt[0] != nullptr) {
    return false;
  }
  int pizzaCount = 0;
  for (Pizza* p : belt) {
    if (p != nullptr) pizzaCount++;
  }
  if (pizzaCount >= length * 0.85f) {
    decreaseDurability(1.0f);
  }
  belt[0] = pizza;
  return true;
}

Pizza* ConveyorMachine::advance() {
  Pizza* ejected = belt[length - 1];
  for (int i = length - 1; i > 0; --i) {
    belt[i] = belt[i - 1];
  }
  belt[0] = nullptr;
  return ejected;
}

bool ConveyorMachine::hasPizzaToEject() const {
  return belt[length - 1] != nullptr;
}

Pizza* ConveyorMachine::ejectPizza() {
  return advance();
}

// =============================================================================
// ConveyorBelt
// =============================================================================
void ConveyorBelt::process() {
  if (!isPoweredOn) return;
  // 마지막 슬롯이 비어있을 때만 벨트를 한 칸 전진시킴
  // (마지막 슬롯에 피자가 있으면 배출 대기 상태이므로 이동하지 않음)
  if (belt[length - 1] == nullptr) {
    for (int i = length - 1; i > 0; --i) {
      belt[i] = belt[i - 1];
    }
    belt[0] = nullptr;
  }
}

void Machine::forceBreak() {
  isBroken = true;
  durability = 0.0f;
  currentRepairTimer = repairTime;
}

void Machine::instantRepair() {
  isBroken = false;
  durability = maxDurability;
  currentRepairTimer = 0;
}

void Machine::resetState() {
  instantRepair();
  isPoweredOn = true;
}

void NonConveyorMachine::resetState() {
  Machine::resetState();
  for (Pizza* p : pizzasInProcess) delete p;
  pizzasInProcess.clear();
}

void ConveyorMachine::resetState() {
  Machine::resetState();
  for (Pizza* p : belt) {
    if (p) delete p;
  }
  std::fill(belt.begin(), belt.end(), nullptr);
}
