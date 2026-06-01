#pragma once

#include <string>
#include <vector>

#include "machines.h"
#include "order.h"

// =============================================================================
// PizzaFactoryModel - 피자 공장 데이터 모델
// 앱에서 사용하는 모든 상태 데이터를 여기에 정의합니다.
// =============================================================================
class PizzaFactoryModel {
private:
  OrderManager* orderManager;
  bool isSpawningEnabled;
  bool isRunning;
  float simulationSpeed;
  int breakdownCount;
  int totalEarnings;

  std::vector<Machine*> pipeline;
  std::vector<Pizza*> finishedPizzas;
  std::vector<Pizza*> lostPizzas;
  int nextPizzaId = 1;

public:

  PizzaFactoryModel();
  ~PizzaFactoryModel();

  void InitDefaultPipeline();

  OrderManager* getOrderManager() const { return orderManager; }
  bool getIsSpawningEnabled() const { return isSpawningEnabled; }
  void setIsSpawningEnabled(bool enabled) { isSpawningEnabled = enabled; }

  bool getIsRunning() const { return isRunning; }
  void setIsRunning(bool running) { isRunning = running; }

  float getSimulationSpeed() const { return simulationSpeed; }
  void setSimulationSpeed(float speed) { simulationSpeed = speed; }

  int getBreakdownCount() const { return breakdownCount; }
  void incrementBreakdownCount() { breakdownCount++; }
  void resetBreakdownCount() { breakdownCount = 0; }

  int getTotalEarnings() const { return totalEarnings; }
  void addEarnings(int amount) { totalEarnings += amount; }
  void resetEarnings() { totalEarnings = 0; }

  const std::vector<Machine*>& getPipeline() const { return pipeline; }
  const std::vector<Pizza*>& getFinishedPizzas() const { return finishedPizzas; }
  const std::vector<Pizza*>& getLostPizzas() const { return lostPizzas; }

  void addFinishedPizza(Pizza* pizza) { finishedPizzas.push_back(pizza); }
  void addLostPizza(Pizza* pizza) { lostPizzas.push_back(pizza); }
  int generateNextPizzaId() { return nextPizzaId++; }
  void resetFinishedAndLostPizzas() {
    for (Pizza* p : finishedPizzas) delete p;
    finishedPizzas.clear();
    for (Pizza* p : lostPizzas) delete p;
    lostPizzas.clear();
    nextPizzaId = 1;
  }
};
