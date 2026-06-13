#include "app.h"
#include "imgui.h"

#include "models/factory.h"
#include "controllers/factory_controller.h"
#include "views/dashboard_view.h"

// =============================================================================
//  Integration seam — the only file that sees both sides (backend Factory + UI View).
//
//  Every frame:
//    1. snapshot()  -> read-only snapshot
//    2. view.Render(snap, cmd) -> buttons mark the cmd flags
//    3. controller.applyCmd(cmd) -> maps cmd to Factory control methods
//    4. cmd = {}  -> cleared immediately so it stays valid for one frame only
//    5. controller.advance(dt) -> if running, steps in proportion to speed
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
    // The Factory destructor releases the pipeline resources.
}
