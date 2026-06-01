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
  for (Machine* m : pipeline) {
    delete m;
  }
  for (Pizza* p : finishedPizzas) {
    delete p;
  }
  for (Pizza* p : lostPizzas) {
    delete p;
  }
}

void PizzaFactoryModel::InitDefaultPipeline() {
  // 1. 도우 스트레쳐
  pipeline.push_back(new DoughStretcher());
  // 2. 컨베이어 벨트
  pipeline.push_back(new ConveyorBelt());
  // 3. 소스 스프레더
  pipeline.push_back(new SauceSpreader());
  // 4. 컨베이어 벨트
  pipeline.push_back(new ConveyorBelt());
  // 5. 치즈 스프레더
  pipeline.push_back(new CheeseSpreader());
  // 6. 컨베이어 벨트
  pipeline.push_back(new ConveyorBelt());
  // 7. 커터
  pipeline.push_back(new Cutter());
  // 8. 컨베이어 벨트
  pipeline.push_back(new ConveyorBelt());
  // 9. 오븐
  pipeline.push_back(new Oven());
  // 10. 컨베이어 벨트
  pipeline.push_back(new ConveyorBelt());
  // 11. 토핑 어플라이어
  pipeline.push_back(new ToppingApplier());
  // 12. 컨베이어 벨트
  pipeline.push_back(new ConveyorBelt());
  // 13. 포장기
  pipeline.push_back(new PackagingMachine());
  // 14. 컨베이어 벨트
  pipeline.push_back(new ConveyorBelt());
}
