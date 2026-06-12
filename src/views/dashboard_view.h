#pragma once
// =============================================================================
// DashboardView — 피자 공장 대시보드 (snapshot 그림 / cmd 표시)
//
//   * 백엔드 타입을 모른다. 오직 bridge.h(값 구조체) + imgui 만 include.
//   * Render(snap, cmd): snap 으로 그리고, 버튼은 cmd 플래그에 "표시"만 한다.
//   * 채점 기준(factory_project_v3) 필수 5창을 각각 독립 ImGui 창으로 렌더한다.
// =============================================================================
#include "imgui.h"
#include "../bridge.h"

class DashboardView
{
public:
    /// ImGui 프레임 내에서 매 프레임 호출. cmd 는 호출측이 매 프레임 비워서 넘김.
    void Render(const FactorySnap& snap, FactoryCmd& cmd);

private:
    void RenderControl   (const FactorySnap& snap, FactoryCmd& cmd);
    void RenderFloor     (const FactorySnap& snap, FactoryCmd& cmd);
    void RenderInspector (const FactorySnap& snap, FactoryCmd& cmd);
    void RenderEventLog  (const FactorySnap& snap, FactoryCmd& cmd);
    void RenderStatistics(const FactorySnap& snap, FactoryCmd& cmd);
    void RenderOrders    (const FactorySnap& snap, FactoryCmd& cmd);

    int  m_selected    = -1;     // 선택 머신 인덱스 (-1 = 없음)
    int  m_speedUI     = 1;      // 배속 슬라이더 상태 (1..5)
    bool m_autoScroll  = true;   // 이벤트 로그 자동 스크롤
    bool m_firstFrame  = true;
};
