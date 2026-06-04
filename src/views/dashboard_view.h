#pragma once
#include "imgui.h"

class PizzaFactoryModel;
class FactoryController;

// =============================================================================
// DashboardView - 피자 공장 대시보드 UI
// 모델의 데이터를 ImGui로 렌더링합니다.
// 사용자 입력은 컨트롤러를 통해 모델에 반영됩니다.
// =============================================================================
class DashboardView
{
public:
    void Init(PizzaFactoryModel* model, FactoryController* controller);

    /// ImGui 프레임 내에서 호출
    void Render();

private:
    void RenderMachineSettingsPanel();

    PizzaFactoryModel*  m_model      = nullptr;
    FactoryController*  m_controller = nullptr;

    int     m_selectedMachineIdx = -1;   // -1 = 선택 없음
    ImVec2  m_settingsPanelPos;          // 설정 창 위치
};
