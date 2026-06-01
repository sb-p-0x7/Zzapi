#pragma once

class PizzaFactoryModel;

// =============================================================================
// FactoryController - 피자 공장 비즈니스 로직
// 모델을 조작하는 모든 로직이 여기에 들어갑니다.
// =============================================================================
class FactoryController
{
public:
    void Init(PizzaFactoryModel* model);

    /// 매 프레임 업데이트
    void Update();

    void togglePlayPause();
    void resetSimulation();
    void forceBreakMachine(int idx);
    void instantRepairMachine(int idx);
    void toggleMachinePower(int idx);

private:
    void SingleTick();

private:
    PizzaFactoryModel* m_model = nullptr;
    int m_frameCount = 0;
    float m_tickAccumulator = 0.0f;
};
