#include "pizza_factory_model.h"

PizzaFactoryModel::PizzaFactoryModel() {
  orderManager = new OrderManager();
  isSpawningEnabled = false; // 기본으로 꺼둠
  isRunning = false;
  simulationSpeed = 1.0f;
  breakdownCount = 0;
  totalEarnings = 0;
  InitDefaultPipeline();
}

PizzaFactoryModel::~PizzaFactoryModel() {
  delete orderManager;
  for (Machine *m : pipeline) {
    delete m;
  }
  for (Pizza *p : finishedPizzas) {
    delete p;
  }
  for (Pizza *p : lostPizzas) {
    delete p;
  }
}

void PizzaFactoryModel::InitDefaultPipeline() {
  // 파이프라인 순서 정의 — 여기만 수정하면 됩니다.
  std::vector<Machine *> sequence = {
      new DoughStretcher(),
      new SauceSpreader(),
      new CheeseSpreader(),
      new ToppingApplier(1.0f, 100.0f, 1, {ToppingType::PEPPERONI}),
      new Oven(),
      new Cutter(),
      new PackagingMachine(),
  };

  // 머신 사이사이에 컨베이어 벨트를 자동으로 삽입
  for (Machine *machine : sequence) {
    pipeline.push_back(machine);
    pipeline.push_back(new ConveyorBelt());
  }
}
