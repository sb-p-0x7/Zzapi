#include "dashboard_view.h"
#include "../models/pizza_factory_model.h"
#include "../controllers/factory_controller.h"
#include "../models/machines.h"
#include "imgui.h"
#include <cmath>
#include <string>
#include <vector>

// =============================================================================
// Helper Drawing Functions
// =============================================================================

static void DrawPizza(ImDrawList* drawList, ImVec2 center, Pizza* pizza) {
    if (!pizza) return;
    
    // 1. Draw Dough base
    float r = 16.0f; // Medium
    if (pizza->getSize() == PizzaSize::SMALL) r = 12.0f;
    else if (pizza->getSize() == PizzaSize::LARGE) r = 20.0f;
    
    ImU32 doughColor = ImColor(245, 222, 179); // Cream (RAW)
    if (pizza->getDoughState() == DoughState::STRETCHED) {
        doughColor = ImColor(255, 239, 213); // Lighter cream
    } else if (pizza->getDoughState() == DoughState::BAKED || pizza->getIsBaked()) {
        doughColor = ImColor(210, 150, 75); // Golden brown (BAKED)
    }
    
    // Draw crust border
    drawList->AddCircleFilled(center, r, doughColor);
    drawList->AddCircle(center, r, ImColor(139, 90, 43), 0, 1.5f); // Crust line
    
    // 2. Draw Sauce
    if (pizza->getHasSauce()) {
        drawList->AddCircleFilled(center, r - 3.0f, ImColor(200, 40, 40)); // Red sauce
    }
    
    // 3. Draw Cheese
    if (pizza->getHasCheese()) {
        drawList->AddCircleFilled(center, r - 5.0f, ImColor(255, 220, 100, 220)); // Yellow cheese
    }
    
    // 4. Draw Toppings
    if (pizza->getHasTopping()) {
        float dist = r - 7.0f;
        if (dist > 3.0f) {
            drawList->AddCircleFilled(ImVec2(center.x - dist/2, center.y - dist/2), 2.5f, ImColor(150, 20, 20)); // Pepperoni
            drawList->AddCircleFilled(ImVec2(center.x + dist/2, center.y - dist/2), 2.5f, ImColor(150, 20, 20));
            drawList->AddCircleFilled(ImVec2(center.x - dist/2, center.y + dist/2), 2.5f, ImColor(150, 20, 20));
            drawList->AddCircleFilled(ImVec2(center.x + dist/2, center.y + dist/2), 2.5f, ImColor(150, 20, 20));
            drawList->AddCircleFilled(center, 2.5f, ImColor(150, 20, 20));
        }
    }
    
    // 5. Draw Slices (Cut marks)
    if (pizza->getIsCut()) {
        int count = pizza->getSliceCount();
        if (count <= 0) count = 8; // Default to 8
        for (int i = 0; i < count / 2; ++i) {
            float angle = i * (3.14159265f / (count / 2));
            ImVec2 d(cos(angle) * r, sin(angle) * r);
            drawList->AddLine(ImVec2(center.x - d.x, center.y - d.y), ImVec2(center.x + d.x, center.y + d.y), ImColor(50, 30, 10, 150), 1.0f);
        }
    }
    
    // 6. Draw Packaging box
    if (pizza->getIsPackaged()) {
        drawList->AddRect(ImVec2(center.x - r - 4, center.y - r - 4), ImVec2(center.x + r + 4, center.y + r + 4), ImColor(180, 130, 90), 4.0f, 0, 2.0f);
    }
}

static void DrawConveyorBelt(ImDrawList* drawList, ImVec2 start, ImVec2 end, bool reverse = false) {
    // Draw thick gray belt background
    drawList->AddLine(start, end, ImColor(65, 70, 75), 32.0f);
    
    // Draw borders
    ImVec2 dir = ImVec2(end.x - start.x, end.y - start.y);
    float len = sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.0f) {
        dir.x /= len;
        dir.y /= len;
        ImVec2 normal(-dir.y, dir.x);
        
        drawList->AddLine(ImVec2(start.x + normal.x * 16.0f, start.y + normal.y * 16.0f), 
                          ImVec2(end.x + normal.x * 16.0f, end.y + normal.y * 16.0f), ImColor(40, 42, 45), 2.5f);
        drawList->AddLine(ImVec2(start.x - normal.x * 16.0f, start.y - normal.y * 16.0f), 
                          ImVec2(end.x - normal.x * 16.0f, end.y - normal.y * 16.0f), ImColor(40, 42, 45), 2.5f);
        
        // Draw moving slits for the conveyor texture (Micro-animation)
        float speed = 30.0f;
        float timeOffset = fmod(ImGui::GetTime() * speed, 16.0f);
        if (reverse) {
            timeOffset = 16.0f - timeOffset;
        }
        for (float dist = timeOffset; dist < len; dist += 16.0f) {
            ImVec2 lineCenter(start.x + dir.x * dist, start.y + dir.y * dist);
            drawList->AddLine(ImVec2(lineCenter.x - normal.x * 10.0f, lineCenter.y - normal.y * 10.0f),
                              ImVec2(lineCenter.x + normal.x * 10.0f, lineCenter.y + normal.y * 10.0f), ImColor(50, 55, 60), 2.0f);
        }
    }
}

static void DrawMachineNode(ImDrawList* drawList, ImVec2 center, Machine* machine, const char* nameKo, const char* iconStr, int pipelineIdx) {
    ImVec2 pMin(center.x - 45, center.y - 45);
    ImVec2 pMax(center.x + 45, center.y + 45);
    
    // Bounding box colors
    ImU32 bgColor = ImColor(38, 48, 58);
    ImU32 borderColor = ImColor(110, 120, 130);
    float borderWidth = 2.0f;
    
    if (machine->getIsBroken()) {
        // Flashing red warn (Micro-animation)
        float pulse = 0.5f + 0.5f * sin(ImGui::GetTime() * 12.0f);
        borderColor = ImColor(255, (int)(40 + 60 * pulse), (int)(40 + 60 * pulse));
        borderWidth = 3.0f;
    } else if (!machine->getIsPoweredOn()) {
        borderColor = ImColor(70, 75, 80);
        bgColor = ImColor(26, 30, 34);
    } else {
        borderColor = ImColor(52, 152, 219); // Active cyan-blue
    }
    
    // Draw base card
    drawList->AddRectFilled(pMin, pMax, bgColor, 12.0f);
    drawList->AddRect(pMin, pMax, borderColor, 12.0f, 0, borderWidth);
    
    // Draw contents
    if (machine->getIsBroken()) {
        drawList->AddText(ImGui::GetFont(), 30.0f, ImVec2(center.x - 15, center.y - 25), ImColor(255, 60, 60), "⚠️");
        
        char timerBuf[32];
        snprintf(timerBuf, sizeof(timerBuf), "수리중 %df", machine->getCurrentRepairTimer());
        ImVec2 timerTextSize = ImGui::CalcTextSize(timerBuf);
        drawList->AddText(ImGui::GetFont(), 13.0f, ImVec2(center.x - timerTextSize.x / 2.0f, center.y + 10), ImColor(255, 120, 120), timerBuf);
    } else {
        // Normal state
        drawList->AddText(ImGui::GetFont(), 28.0f, ImVec2(center.x - 14, center.y - 24), 
                          machine->getIsPoweredOn() ? ImColor(255, 255, 255) : ImColor(120, 120, 120), iconStr);
        
        ImVec2 text_size = ImGui::CalcTextSize(nameKo);
        drawList->AddText(ImGui::GetFont(), 13.0f, ImVec2(center.x - text_size.x / 2.0f, center.y + 12), 
                          machine->getIsPoweredOn() ? ImColor(230, 240, 250) : ImColor(100, 105, 110), nameKo);
    }
    
    // Draw Durability health-bar
    float durabilityPct = machine->getDurability() / machine->getMaxDurability();
    if (durabilityPct < 0.0f) durabilityPct = 0.0f;
    ImU32 barColor = ImColor(46, 204, 113); // Green
    if (durabilityPct < 0.3f) barColor = ImColor(231, 76, 60); // Red
    else if (durabilityPct < 0.8f) barColor = ImColor(241, 196, 15); // Yellow/Orange
    
    ImVec2 barMin(center.x - 35, center.y + 32);
    ImVec2 barMax(center.x + 35, center.y + 36);
    drawList->AddRectFilled(barMin, barMax, ImColor(40, 40, 40), 2.0f); // background
    if (durabilityPct > 0.0f) {
        drawList->AddRectFilled(barMin, ImVec2(barMin.x + 70.0f * durabilityPct, barMax.y), barColor, 2.0f);
    }
    
    // Draw pizza inside NonConveyorMachine
    auto* ncm = dynamic_cast<NonConveyorMachine*>(machine);
    if (ncm && !machine->getIsBroken()) {
        const auto& pizzas = ncm->getPizzasInProcess();
        if (!pizzas.empty() && pizzas[0] != nullptr) {
            DrawPizza(drawList, center, pizzas[0]);
        }
    }
    
    // Clickable area for Popups
    ImGui::SetCursorScreenPos(ImVec2(center.x - 45, center.y - 45));
    char btnId[64];
    snprintf(btnId, sizeof(btnId), "##btn_%d", pipelineIdx);
    if (ImGui::InvisibleButton(btnId, ImVec2(90, 90))) {
        ImGui::OpenPopup(btnId);
    }
    
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        machine->togglePower();
    }
    
    // Popup Menu config
    if (ImGui::BeginPopup(btnId)) {
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 0.9f, 1.0f), "⚙️ %s 설정", nameKo);
        ImGui::Separator();
        
        bool power = machine->getIsPoweredOn();
        if (ImGui::Checkbox("전원 가동 (Power On)", &power)) {
            machine->setPower(power);
        }
        
        if (auto* stretcher = dynamic_cast<DoughStretcher*>(machine)) {
            ImGui::Text("도우 타겟 크기:");
            int currentSize = (int)stretcher->getTargetSize();
            if (ImGui::RadioButton("Small (S)", currentSize == 0)) stretcher->setTargetSize(PizzaSize::SMALL);
            if (ImGui::RadioButton("Medium (M)", currentSize == 1)) stretcher->setTargetSize(PizzaSize::MEDIUM);
            if (ImGui::RadioButton("Large (L)", currentSize == 2)) stretcher->setTargetSize(PizzaSize::LARGE);
        }
        else if (auto* cutter = dynamic_cast<Cutter*>(machine)) {
            ImGui::Text("커터 조각수:");
            int slices = cutter->getSliceCount();
            if (ImGui::SliderInt("조각 수", &slices, 4, 12)) {
                if (slices % 2 != 0) slices++;
                cutter->setSliceCount(slices);
            }
        }
        else if (auto* oven = dynamic_cast<Oven*>(machine)) {
            ImGui::Text("오븐 온도 설정:");
            float temp = oven->getTemperature();
            if (ImGui::SliderFloat("온도 (°C)", &temp, 150.0f, 350.0f, "%.0f")) {
                oven->setTemperature(temp);
            }
        }
        
        ImGui::Separator();
        ImGui::Text("기계 내구도: %.1f%%", machine->getDurability());
        ImGui::EndPopup();
    }
}

static void DrawDoughStorageVisual(ImDrawList* drawList, ImVec2 center) {
    ImVec2 pMin(center.x - 45, center.y - 45);
    ImVec2 pMax(center.x + 45, center.y + 45);
    
    drawList->AddRectFilled(pMin, pMax, ImColor(46, 56, 66), 12.0f);
    drawList->AddRect(pMin, pMax, ImColor(120, 130, 140), 12.0f, 0, 2.0f);
    
    drawList->AddText(ImGui::GetFont(), 28.0f, ImVec2(center.x - 14, center.y - 25), ImColor(255, 255, 255), "🌾");
    
    // Draw flour dough balls
    drawList->AddCircleFilled(ImVec2(center.x - 18, center.y + 8), 9.0f, ImColor(250, 240, 220));
    drawList->AddCircleFilled(ImVec2(center.x + 18, center.y + 8), 9.0f, ImColor(250, 240, 220));
    drawList->AddCircleFilled(ImVec2(center.x, center.y + 16), 9.0f, ImColor(250, 240, 220));
    
    ImVec2 text_size = ImGui::CalcTextSize("도우 보관소");
    drawList->AddText(ImGui::GetFont(), 13.0f, ImVec2(center.x - text_size.x / 2.0f, center.y - 6), ImColor(220, 220, 220), "도우 보관소");
}

static void DrawCounterVisual(ImDrawList* drawList, ImVec2 center, PizzaFactoryModel* model) {
    ImVec2 pMin(center.x - 45, center.y - 45);
    ImVec2 pMax(center.x + 45, center.y + 45);
    
    drawList->AddRectFilled(pMin, pMax, ImColor(58, 48, 38), 12.0f);
    drawList->AddRect(pMin, pMax, ImColor(160, 130, 90), 12.0f, 0, 2.0f);
    
    drawList->AddText(ImGui::GetFont(), 28.0f, ImVec2(center.x - 14, center.y - 25), ImColor(255, 220, 100), "🛎️");
    
    ImVec2 text_size = ImGui::CalcTextSize("카운터");
    drawList->AddText(ImGui::GetFont(), 13.0f, ImVec2(center.x - text_size.x / 2.0f, center.y + 12), ImColor(220, 220, 220), "카운터");
    
    if (!model->getFinishedPizzas().empty()) {
        // Visual pizza box stacked at delivery
        drawList->AddRectFilled(ImVec2(center.x - 22, center.y - 4), ImVec2(center.x + 22, center.y + 6), ImColor(180, 130, 90), 3.0f);
        drawList->AddRect(ImVec2(center.x - 22, center.y - 4), ImVec2(center.x + 22, center.y + 6), ImColor(130, 90, 60), 3.0f, 0, 1.5f);
    }
}

// Calculate the belt path slot position
static ImVec2 GetBeltPosition(int pipelineIdx, float t, ImVec2 origin) {
    ImVec2 p;
    if (pipelineIdx == 1) { // Stretcher (260,100) to Sauce (420,100)
        p.x = 260.0f + (420.0f - 260.0f) * t;
        p.y = 100.0f;
    } else if (pipelineIdx == 3) { // Sauce (420,100) to Cheese (580,100)
        p.x = 420.0f + (580.0f - 420.0f) * t;
        p.y = 100.0f;
    } else if (pipelineIdx == 5) { // Curve Cheese (580,100) to Cutter (580,260) via (680,100) and (680,260)
        float dist = t * 360.0f;
        if (dist < 100.0f) {
            p.x = 580.0f + dist;
            p.y = 100.0f;
        } else if (dist < 260.0f) {
            p.x = 680.0f;
            p.y = 100.0f + (dist - 100.0f);
        } else {
            p.x = 680.0f - (dist - 260.0f);
            p.y = 260.0f;
        }
    } else if (pipelineIdx == 7) { // Cutter (580,260) to Oven (420,260) (flowing right to left)
        p.x = 580.0f - (580.0f - 420.0f) * t;
        p.y = 260.0f;
    } else if (pipelineIdx == 9) { // Oven (420,260) to Topping (260,260) (flowing right to left)
        p.x = 420.0f - (420.0f - 260.0f) * t;
        p.y = 260.0f;
    } else if (pipelineIdx == 11) { // Curve Topping (260,260) to Packaging (260,420) via (160,260) and (160,420)
        float dist = t * 360.0f;
        if (dist < 100.0f) {
            p.x = 260.0f - dist;
            p.y = 260.0f;
        } else if (dist < 260.0f) {
            p.x = 160.0f;
            p.y = 260.0f + (dist - 100.0f);
        } else {
            p.x = 160.0f + (dist - 260.0f);
            p.y = 420.0f;
        }
    } else if (pipelineIdx == 13) { // Packaging (260,420) to Counter (420,420)
        p.x = 260.0f + (420.0f - 260.0f) * t;
        p.y = 420.0f;
    } else {
        p = ImVec2(100.0f, 100.0f);
    }
    return ImVec2(origin.x + p.x, origin.y + p.y);
}

// =============================================================================
// DashboardView Implementation
// =============================================================================

void DashboardView::Init(PizzaFactoryModel* model, FactoryController* controller)
{
    m_model = model;
    m_controller = controller;
}

void DashboardView::Render()
{
    if (!m_model || !m_controller) return;

    ImGui::SetNextWindowSize(ImVec2(1250, 640), ImGuiCond_FirstUseEver);
    ImGui::Begin("🍕 자동 피자 공장 운영 시뮬레이터 Dashboard");

    // -------------------------------------------------------------------------
    // Top Control and Stats Panel
    // -------------------------------------------------------------------------
    OrderManager* om = m_model->getOrderManager();
    int completedCount = om->getCompletedOrders().size();
    int failedCount = om->getFailedOrders().size();
    int lostCount = m_model->getLostPizzas().size();
    
    int totalGold = 0;
    for (Order* o : om->getCompletedOrders()) {
        totalGold += o->getReward();
    }

    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "🏭 피자 공장 제어판");
    ImGui::SameLine(180);

    // Play/Pause Button
    bool isRunning = m_model->getIsRunning();
    if (isRunning) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("⏸ 일시정지 (Pause)")) {
            m_controller->togglePlayPause();
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        if (ImGui::Button("▶ 시작 (Start)")) {
            m_controller->togglePlayPause();
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine(320);
    // Reset Button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.4f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.5f, 0.1f, 1.0f));
    if (ImGui::Button("🔄 리셋 (Reset)")) {
        m_controller->resetSimulation();
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine(430);
    // Spawner toggle
    bool spawn = m_model->getIsSpawningEnabled();
    if (ImGui::Checkbox("자동 공급 (Spawner)", &spawn)) {
        m_model->setIsSpawningEnabled(spawn);
    }

    ImGui::SameLine(610);
    // Speed Slider
    float speed = m_model->getSimulationSpeed();
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderFloat("속도", &speed, 0.1f, 5.0f, "%.1fx")) {
        m_model->setSimulationSpeed(speed);
    }

    ImGui::SameLine(820);
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "🪙 $%d", m_model->getTotalEarnings());
    ImGui::SameLine(920);
    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "✅ 납품: %d", completedCount);
    ImGui::SameLine(1020);
    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.2f, 1.0f), "❌ 초과: %d", failedCount);
    ImGui::SameLine(1120);
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.5f, 1.0f), "⚙️ 고장: %d", m_model->getBreakdownCount());

    ImGui::Separator();
    ImGui::Spacing();

    // -------------------------------------------------------------------------
    // Left Canvas Child: S-curve pipeline graphic
    // -------------------------------------------------------------------------
    ImGui::BeginChild("FactoryCanvasChild", ImVec2(900, 530), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    // Draw grid background canvas
    drawList->AddRectFilled(origin, ImVec2(origin.x + 880, origin.y + 510), ImColor(30, 34, 40), 12.0f);
    
    // Draw grid subtle guide lines
    for (float g = 40.0f; g < 880.0f; g += 40.0f) {
        drawList->AddLine(ImVec2(origin.x + g, origin.y), ImVec2(origin.x + g, origin.y + 510), ImColor(40, 45, 50, 100), 1.0f);
    }
    for (float g = 40.0f; g < 510.0f; g += 40.0f) {
        drawList->AddLine(ImVec2(origin.x, origin.y + g), ImVec2(origin.x + 880, origin.y + g), ImColor(40, 45, 50, 100), 1.0f);
    }

    // 1. Draw Visual Belts (Paths)
    // Dough Storage to Stretcher
    DrawConveyorBelt(drawList, ImVec2(origin.x + 100, origin.y + 100), ImVec2(origin.x + 260, origin.y + 100));
    
    // Index 1: Stretcher to Sauce
    DrawConveyorBelt(drawList, ImVec2(origin.x + 260, origin.y + 100), ImVec2(origin.x + 420, origin.y + 100));
    
    // Index 3: Sauce to Cheese
    DrawConveyorBelt(drawList, ImVec2(origin.x + 420, origin.y + 100), ImVec2(origin.x + 580, origin.y + 100));
    
    // Index 5: Curve Cheese to Cutter (Right U-Turn)
    DrawConveyorBelt(drawList, ImVec2(origin.x + 580, origin.y + 100), ImVec2(origin.x + 680, origin.y + 100));
    DrawConveyorBelt(drawList, ImVec2(origin.x + 680, origin.y + 100), ImVec2(origin.x + 680, origin.y + 260));
    DrawConveyorBelt(drawList, ImVec2(origin.x + 680, origin.y + 260), ImVec2(origin.x + 580, origin.y + 260), true); // reverse anim
    
    // Index 7: Cutter to Oven (Flowing Right-to-Left)
    DrawConveyorBelt(drawList, ImVec2(origin.x + 580, origin.y + 260), ImVec2(origin.x + 420, origin.y + 260), true);
    
    // Index 9: Oven to Topping (Flowing Right-to-Left)
    DrawConveyorBelt(drawList, ImVec2(origin.x + 420, origin.y + 260), ImVec2(origin.x + 260, origin.y + 260), true);
    
    // Index 11: Curve Topping to Packaging (Left U-Turn)
    DrawConveyorBelt(drawList, ImVec2(origin.x + 260, origin.y + 260), ImVec2(origin.x + 160, origin.y + 260), true);
    DrawConveyorBelt(drawList, ImVec2(origin.x + 160, origin.y + 260), ImVec2(origin.x + 160, origin.y + 420));
    DrawConveyorBelt(drawList, ImVec2(origin.x + 160, origin.y + 420), ImVec2(origin.x + 260, origin.y + 420));
    
    // Index 13: Packaging to Counter
    DrawConveyorBelt(drawList, ImVec2(origin.x + 260, origin.y + 420), ImVec2(origin.x + 420, origin.y + 420));

    // 2. Draw Moving Pizzas on the Belts
    const auto& pipeline = m_model->getPipeline();
    for (int i = 0; i < (int)pipeline.size(); ++i) {
        auto* belt = dynamic_cast<ConveyorMachine*>(pipeline[i]);
        if (belt) {
            const auto& slots = belt->getBelt();
            int len = slots.size();
            for (int s = 0; s < len; ++s) {
                if (slots[s] != nullptr) {
                    float t = (s + 0.5f) / len;
                    ImVec2 pizzaPos = GetBeltPosition(i, t, origin);
                    DrawPizza(drawList, pizzaPos, slots[s]);
                }
            }
        }
    }

    // 3. Draw Static Spawner & Counter visuals
    DrawDoughStorageVisual(drawList, ImVec2(origin.x + 100, origin.y + 100));
    DrawCounterVisual(drawList, ImVec2(origin.x + 420, origin.y + 420), m_model);

    // 4. Draw Machines (Casting correctly and mapping coordinates)
    if (pipeline.size() >= 13) {
        DrawMachineNode(drawList, ImVec2(origin.x + 260, origin.y + 100), pipeline[0], "도우 스트레쳐", "🫓", 0);
        DrawMachineNode(drawList, ImVec2(origin.x + 420, origin.y + 100), pipeline[2], "소스 스프레더", "🥫", 2);
        DrawMachineNode(drawList, ImVec2(origin.x + 580, origin.y + 100), pipeline[4], "치즈 스프레더", "🧀", 4);
        DrawMachineNode(drawList, ImVec2(origin.x + 580, origin.y + 260), pipeline[6], "커터", "🔪", 6);
        DrawMachineNode(drawList, ImVec2(origin.x + 420, origin.y + 260), pipeline[8], "오븐", "🔥", 8);
        DrawMachineNode(drawList, ImVec2(origin.x + 260, origin.y + 260), pipeline[10], "토핑 어플라이어", "🍕", 10);
        DrawMachineNode(drawList, ImVec2(origin.x + 260, origin.y + 420), pipeline[12], "페키져", "📦", 12);
    }
    
    // Small guide text inside Canvas
    drawList->AddText(ImGui::GetFont(), 13.0f, ImVec2(origin.x + 15, origin.y + 485), 
                      ImColor(150, 160, 170), "💡 좌클릭: 기계 세부 설정 변경 | 우클릭: 전원(ON/OFF) 빠르게 토글");

    ImGui::EndChild();
    
    ImGui::SameLine();

    // -------------------------------------------------------------------------
    // Right Panel: Active Order Board Slips
    // -------------------------------------------------------------------------
    ImGui::BeginChild("OrderSlipsChild", ImVec2(320, 530), true);
    ImGui::TextColored(ImVec4(0.95f, 0.9f, 0.6f, 1.0f), "🛎️ 주문서 대기열 (Active Orders)");
    ImGui::Separator();
    ImGui::Spacing();

    const std::vector<Order*>& activeOrders = om->getActiveOrders();
    if (activeOrders.empty()) {
        ImGui::Text("대기 중인 주문이 없습니다.");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Spawner 스위치를 켜면");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " 주문이 들어오기 시작합니다)");
    } else {
        for (int i = 0; i < (int)activeOrders.size(); ++i) {
            Order* o = activeOrders[i];
            
            char slipId[64];
            snprintf(slipId, sizeof(slipId), "slip_%d", o->getId());
            
            // Draw a paper-like white container
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
            
            ImGui::BeginChild(slipId, ImVec2(0, 140), true, ImGuiWindowFlags_NoScrollbar);
            
            // Order Header
            ImGui::Text("📝 주문서 #%d", o->getId());
            ImGui::SameLine(180);
            ImGui::TextColored(ImVec4(0.85f, 0.6f, 0.0f, 1.0f), "🪙 $%d", o->getReward());
            ImGui::Separator();
            
            // Display Requirements
            ImGui::Text("크기: %s", (o->getRequiredSize() == PizzaSize::SMALL ? "Small (S)" : 
                                    (o->getRequiredSize() == PizzaSize::MEDIUM ? "Medium (M)" : "Large (L)")));
            
            // Show Colored Badges (Pills)
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
            
            if (o->getRequiresSauce()) {
                ImGui::Button("🥫 소스"); ImGui::SameLine();
            }
            if (o->getRequiresCheese()) {
                ImGui::Button("🧀 치즈"); ImGui::SameLine();
            }
            if (o->getRequiresBake()) {
                ImGui::Button("🔥 굽기"); ImGui::SameLine();
            }
            if (o->getRequiresCut()) {
                ImGui::Button("🔪 컷팅"); ImGui::SameLine();
            }
            if (o->getRequiresTopping()) {
                ImGui::Button("🍕 토핑");
            }
            
            ImGui::PopStyleVar(2);
            ImGui::Spacing();
            
            // Time Left progress bar
            float totalExpectedLimit = 3000.0f; // Max estimated base frames
            float pct = (float)o->getTimeLeft() / totalExpectedLimit;
            if (pct > 1.0f) pct = 1.0f;
            else if (pct < 0.0f) pct = 0.0f;
            
            ImVec4 barColor = ImVec4(0.18f, 0.8f, 0.44f, 1.0f); // Green
            if (pct < 0.3f) barColor = ImVec4(0.9f, 0.17f, 0.15f, 1.0f); // Red
            else if (pct < 0.6f) barColor = ImVec4(0.95f, 0.77f, 0.06f, 1.0f); // Yellow
            
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
            char timeText[32];
            snprintf(timeText, sizeof(timeText), "남은 시간: %.1fs", (float)o->getTimeLeft() / 60.0f);
            ImGui::ProgressBar(pct, ImVec2(-1, 14), timeText);
            ImGui::PopStyleColor();
            
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
            ImGui::Spacing();
        }
    }

    ImGui::EndChild();

    ImGui::End();
}
