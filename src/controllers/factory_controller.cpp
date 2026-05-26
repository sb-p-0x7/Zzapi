#include "factory_controller.h"
#include "../models/pizza_factory_model.h"

void FactoryController::Init(PizzaFactoryModel* model)
{
    m_model = model;
}

void FactoryController::Update()
{
    if (!m_model) return;

    // 1. 새 피자 스폰 로직 (예: 60프레임마다 1개씩 스폰)
    m_frameCount++;
    
    // 주문 매니저 업데이트
    m_model->getOrderManager()->tick();

    if (m_model->getIsSpawningEnabled() && m_frameCount % 60 == 0) {
        if (!m_model->getPipeline().empty()) {
            Machine* firstMachine = m_model->getPipeline().front();
            Pizza* newPizza = new Pizza(m_model->generateNextPizzaId());
            if (firstMachine->getIsBroken() || !firstMachine->insertPizza(newPizza)) {
                // 첫 머신이 고장났거나 꽉 찼다면 로스
                m_model->addLostPizza(newPizza);
            }
        }
    }

    // 2. 파이프라인 역순 업데이트 (뒤에서부터 앞 방향으로 이동 처리)
    int n = m_model->getPipeline().size();
    for (int i = n - 1; i >= 0; --i) {
        Machine* current = m_model->getPipeline()[i];
        
        // 매 프레임 수리 등 상태 업데이트
        current->tick();

        // 고장난 머신은 작동(process)도 배출(eject)도 하지 않고 멈춤
        if (current->getIsBroken()) {
            continue;
        }

        // 먼저 현재 머신의 처리를 수행 (논컨베이어의 경우 속성 변경)
        current->process();

        // 그 다음, 다음 머신으로 넘길 피자가 있는지 확인
        if (current->hasPizzaToEject()) {
            if (i == n - 1) {
                // 마지막 머신이면 배출
                Pizza* finishedPizza = current->ejectPizza();
                if (finishedPizza) {
                    if (m_model->getOrderManager()->verifyPizza(finishedPizza)) {
                        // 주문 완료 처리됨, 피자는 전달되었으므로 메모리 해제
                        delete finishedPizza;
                    } else {
                        // 주문 조건과 맞지 않는 피자는 로스(폐기) 처리
                        m_model->addLostPizza(finishedPizza);
                    }
                }
            } else {
                Machine* next = m_model->getPipeline()[i + 1];
                // 무조건 배출 시도 (로스 허용)
                Pizza* pizza = current->ejectPizza();
                if (pizza) {
                    if (next->getIsBroken() || !next->insertPizza(pizza)) {
                        // 다음 머신이 고장났거나 용량 초과로 인서트 실패 -> 로스 발생!
                        m_model->addLostPizza(pizza);
                    }
                }
            }
        }
    }
}
