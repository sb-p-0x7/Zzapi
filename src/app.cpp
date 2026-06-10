#include "app.h"
#include "imgui.h"

#include "models/factory.h"
#include "controllers/factory_controller.h"
#include "views/dashboard_view.h"

// =============================================================================
//  통합점(seam) — 양쪽(백엔드 Factory + UI View)을 보는 유일한 파일.
//
//  매 프레임:
//    1. snapshot()  → 읽기 전용 스냅샷
//    2. view.Render(snap, cmd) → 버튼이 cmd 플래그에 표시
//    3. controller.applyCmd(cmd) → cmd 를 Factory 제어 메서드로 매핑
//    4. cmd = {}  → 한 프레임만 유효하도록 즉시 비움
//    5. controller.advance(dt) → running 이면 speed 비례로 step 진행
// =============================================================================
static Factory           s_factory;
static FactoryController s_controller;
static DashboardView     s_view;
static FactoryCmd        s_cmd;

void App::Init()
{
    s_controller.Init(&s_factory);
    s_cmd = FactoryCmd{};
}

void App::Update()
{
    FactorySnap snap = s_factory.snapshot();   // 1
    s_view.Render(snap, s_cmd);                // 2
    s_controller.applyCmd(s_cmd);              // 3
    s_cmd = FactoryCmd{};                      // 4
    s_controller.advance(ImGui::GetIO().DeltaTime); // 5
}

void App::Shutdown()
{
    // Factory 소멸자가 파이프라인 자원을 해제한다.
}
