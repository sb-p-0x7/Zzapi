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

    // TODO: 피자 공장 로직 메서드를 여기에 추가하세요.

private:
    PizzaFactoryModel* m_model = nullptr;
    int m_frameCount = 0;
};
