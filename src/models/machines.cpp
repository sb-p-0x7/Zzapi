#include "machines.h"
#include <algorithm>

// =============================================================================
// DoughStretcher::process()
// =============================================================================
void DoughStretcher::process() {
  if (!isPoweredOn) return;
  if (!pizzasInProcess.empty()) m_processTimer++;
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
  if (!pizzasInProcess.empty()) m_processTimer++;
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
  if (!pizzasInProcess.empty()) m_processTimer++;
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
  if (!pizzasInProcess.empty()) m_processTimer++;
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
  if (!pizzasInProcess.empty()) m_processTimer++;
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
  if (!pizzasInProcess.empty()) m_processTimer++;
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
  if (!pizzasInProcess.empty()) m_processTimer++;
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
  return (int)pizzasInProcess.size() < capacity && m_processTimer == 0;
}

bool NonConveyorMachine::insertPizza(Pizza* pizza) {
  if (!canInsert()) return false;
  if ((int)pizzasInProcess.size() >= capacity * 0.85f) {
    decreaseDurability(1.0f);
  }
  pizzasInProcess.push_back(pizza);
  // speed=1 → 120프레임(약 2초), speed=2 → 60프레임(1초)
  m_requiredFrames = std::max(1, (int)(120.0f / speed));
  m_processTimer = 0;
  return true;
}

bool NonConveyorMachine::hasPizzaToEject() const {
  return !pizzasInProcess.empty() && m_processTimer >= m_requiredFrames;
}

Pizza* NonConveyorMachine::ejectPizza() {
  if (!hasPizzaToEject()) return nullptr;
  Pizza* pizza = pizzasInProcess.front();
  pizzasInProcess.erase(pizzasInProcess.begin());
  m_processTimer   = 0;
  m_requiredFrames = 0;
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
  m_tickCounter++;
  // moveSpeed=1 → 20틱마다 한 칸 이동 (눈으로 보이는 속도)
  int interval = std::max(1, (int)(20.0f / moveSpeed));
  if (m_tickCounter < interval) return;
  m_tickCounter = 0;
  // 마지막 슬롯이 비어있을 때만 전진 (배출 대기 중이면 멈춤)
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
