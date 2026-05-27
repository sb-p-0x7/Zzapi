#include "app.h"
#include "views/dashboard_view.h"
#include "controllers/factory_controller.h"
#include "models/pizza_factory_model.h"

// =============================================================================
// 내부 MVC 인스턴스 (필요 시 스마트 포인터로 교체 가능)
// =============================================================================
static PizzaFactoryModel   s_model;
static FactoryController   s_controller;
static DashboardView       s_view;

void App::Init()
{
    s_model = PizzaFactoryModel();
    s_controller.Init(&s_model);
    s_view.Init(&s_model, &s_controller);
}

void App::Update()
{
    s_controller.Update();
    s_view.Render();
}

void App::Shutdown()
{
    // 필요 시 리소스 해제
}
