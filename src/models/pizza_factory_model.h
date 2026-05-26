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

  const std::vector<Machine*>& getPipeline() const { return pipeline; }
  const std::vector<Pizza*>& getFinishedPizzas() const { return finishedPizzas; }
  const std::vector<Pizza*>& getLostPizzas() const { return lostPizzas; }

  void addFinishedPizza(Pizza* pizza) { finishedPizzas.push_back(pizza); }
  void addLostPizza(Pizza* pizza) { lostPizzas.push_back(pizza); }
  int generateNextPizzaId() { return nextPizzaId++; }
};
