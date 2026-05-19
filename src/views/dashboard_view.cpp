#include "dashboard_view.h"
#include "../models/pizza_factory_model.h"
#include "../controllers/factory_controller.h"
#include "imgui.h"

void DashboardView::Init(PizzaFactoryModel* model, FactoryController* controller)
{
    m_model = model;
    m_controller = controller;
}

void DashboardView::Render()
{
    if (!m_model || !m_controller) return;

    ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
    ImGui::Begin("[Pizza Factory]");

    // TODO: 여기에 ImGui UI를 구현하세요.
    // 예:
    //   ImGui::Text("Hello, Pizza Factory!");
    //   if (ImGui::Button("버튼")) { m_controller->SomeAction(); }

    ImGui::End();
}
