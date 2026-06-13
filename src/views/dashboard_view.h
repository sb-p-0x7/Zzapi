#pragma once
// =============================================================================
// DashboardView — pizza-factory dashboard (draws the snapshot / marks cmd)
//
//   * Knows no backend types. Includes only bridge.h (value structs) + imgui.
//   * Render(snap, cmd): draws from snap; buttons only "mark" the cmd flags.
//   * Renders the five required windows (grading rubric factory_project_v3) as independent ImGui windows.
// =============================================================================
#include "imgui.h"
#include "../bridge.h"

class DashboardView
{
public:
    /// Called every frame inside an ImGui frame. The caller passes a cmd cleared each frame.
    void Render(const FactorySnap& snap, FactoryCmd& cmd);

private:
    void RenderControl   (const FactorySnap& snap, FactoryCmd& cmd);
    void RenderFloor     (const FactorySnap& snap, FactoryCmd& cmd);
    void RenderInspector (const FactorySnap& snap, FactoryCmd& cmd);
    void RenderEventLog  (const FactorySnap& snap, FactoryCmd& cmd);
    void RenderStatistics(const FactorySnap& snap, FactoryCmd& cmd);
    void RenderOrders    (const FactorySnap& snap, FactoryCmd& cmd);
    void RenderMachineList (const FactorySnap& snap, FactoryCmd& cmd);

    int  m_selected    = -1;     // selected machine index (-1 = none)
    int  m_speedUI     = 1;      // speed slider state (1..5)
    bool m_autoScroll  = true;   // event-log auto-scroll
    bool m_firstFrame  = true;
};
