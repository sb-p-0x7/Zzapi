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

static bool DrawMachineNode(ImDrawList* drawList, ImVec2 center, Machine* machine, const char* nameKo, const char* iconStr, int pipelineIdx) {
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
    
    // Clickable invisible button over machine node
    ImGui::SetCursorScreenPos(ImVec2(center.x - 45, center.y - 45));
    char btnId[64];
    snprintf(btnId, sizeof(btnId), "##btn_%d", pipelineIdx);
    bool clicked = ImGui::InvisibleButton(btnId, ImVec2(90, 90));

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        machine->togglePower();
    }

    return clicked;
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

static const char* GetMachineNameKo(Machine* m) {
    if (dynamic_cast<DoughStretcher*>(m))   return "도우 스트레쳐";
    if (dynamic_cast<SauceSpreader*>(m))    return "소스 스프레더";
    if (dynamic_cast<CheeseSpreader*>(m))   return "치즈 스프레더";
    if (dynamic_cast<Cutter*>(m))           return "커터";
    if (dynamic_cast<Oven*>(m))             return "오븐";
    if (dynamic_cast<ToppingApplier*>(m))   return "토핑 어플라이어";
    if (dynamic_cast<PackagingMachine*>(m)) return "페키져";
    return "머신";
}

static const char* GetMachineIcon(Machine* m) {
    if (dynamic_cast<DoughStretcher*>(m))   return "🫓";
    if (dynamic_cast<SauceSpreader*>(m))    return "🥫";
    if (dynamic_cast<CheeseSpreader*>(m))   return "🧀";
    if (dynamic_cast<Cutter*>(m))           return "🔪";
    if (dynamic_cast<Oven*>(m))             return "🔥";
    if (dynamic_cast<ToppingApplier*>(m))   return "🍕";
    if (dynamic_cast<PackagingMachine*>(m)) return "📦";
    return "⚙️";
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
    // Left Canvas Child: dynamic snake pipeline graphic
    // -------------------------------------------------------------------------

    // Layout constants
    const int   MACHINES_PER_ROW = 3;
    const float SLOT_PX          = 50.0f;  // pixels per belt slot
    const float MACHINE_R        = 45.0f;
    const float ROW_DY           = 150.0f;
    const float UTURN_DX         = 65.0f;
    const float PAD_X            = 180.0f;
    const float PAD_Y            = 90.0f;
    const float SPAWNER_BELT_PX  = 60.0f;

    const auto& pipeline = m_model->getPipeline();

    // Collect non-conveyor machines (even indices) and their following belt
    struct MachInfo { Machine* machine; int pipeIdx; ConveyorMachine* nextBelt; };
    std::vector<MachInfo> machList;
    for (int i = 0; i < (int)pipeline.size(); i += 2) {
        ConveyorMachine* belt = (i + 1 < (int)pipeline.size())
            ? dynamic_cast<ConveyorMachine*>(pipeline[i + 1]) : nullptr;
        machList.push_back({pipeline[i], i, belt});
    }
    int N = (int)machList.size();

    // Compute machine center positions (snake layout)
    std::vector<ImVec2> centers(N);
    {
        float cx = PAD_X, cy = PAD_Y;
        int dir = 1;
        for (int i = 0; i < N; i++) {
            centers[i] = {cx, cy};
            if (i + 1 < N) {
                float beltPx = machList[i].nextBelt
                    ? machList[i].nextBelt->getLength() * SLOT_PX : SLOT_PX * 3;
                bool lastInRow = ((i % MACHINES_PER_ROW) == MACHINES_PER_ROW - 1);
                if (lastInRow) { cy += ROW_DY; dir *= -1; }
                else           { cx += dir * (MACHINE_R * 2.0f + beltPx); }
            }
        }
    }

    // Spawner / counter positions
    ImVec2 spawnerCenter = {centers[0].x - MACHINE_R - SPAWNER_BELT_PX - MACHINE_R, centers[0].y};
    int    lastRow       = (N - 1) / MACHINES_PER_ROW;
    int    lastDir       = (lastRow % 2 == 0) ? 1 : -1;
    float  lastBeltPx    = machList[N-1].nextBelt
        ? machList[N-1].nextBelt->getLength() * SLOT_PX : SLOT_PX * 3;
    ImVec2 counterCenter = {
        centers[N-1].x + lastDir * (MACHINE_R + lastBeltPx + MACHINE_R),
        centers[N-1].y
    };

    // Compute canvas size to fit layout
    float canvasW = 880.0f;
    float canvasH = std::max(510.0f, PAD_Y + (float)((N-1)/MACHINES_PER_ROW) * ROW_DY + MACHINE_R + PAD_Y);

    ImGui::BeginChild("FactoryCanvasChild", ImVec2(900, canvasH + 20.0f), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 origin        = ImGui::GetCursorScreenPos();

    // Background + grid
    drawList->AddRectFilled(origin, ImVec2(origin.x + canvasW, origin.y + canvasH), ImColor(30, 34, 40), 12.0f);
    for (float g = 40.0f; g < canvasW; g += 40.0f)
        drawList->AddLine(ImVec2(origin.x + g, origin.y), ImVec2(origin.x + g, origin.y + canvasH), ImColor(40, 45, 50, 100), 1.0f);
    for (float g = 40.0f; g < canvasH; g += 40.0f)
        drawList->AddLine(ImVec2(origin.x, origin.y + g), ImVec2(origin.x + canvasW, origin.y + g), ImColor(40, 45, 50, 100), 1.0f);

    // Offset all positions by canvas origin
    auto O = [&](ImVec2 p) { return ImVec2(origin.x + p.x, origin.y + p.y); };

    // ── Draw belts ──────────────────────────────────────────────────────────
    // Spawner → machine 0
    DrawConveyorBelt(drawList, O({spawnerCenter.x + MACHINE_R, spawnerCenter.y}),
                               O({centers[0].x - MACHINE_R,   centers[0].y}));

    // Machine-to-machine belts
    for (int i = 0; i + 1 < N; i++) {
        if (!machList[i].nextBelt) continue;
        int  row       = i / MACHINES_PER_ROW;
        bool evenRow   = (row % 2 == 0);
        int  rowDir    = evenRow ? 1 : -1;
        bool lastInRow = ((i % MACHINES_PER_ROW) == MACHINES_PER_ROW - 1);
        ImVec2 cA = centers[i], cB = centers[i + 1];

        if (lastInRow) {
            // U-turn: horizontal stub → vertical → horizontal stub (reverse)
            float xTurn = cA.x + rowDir * (MACHINE_R + UTURN_DX);
            DrawConveyorBelt(drawList, O({cA.x + rowDir * MACHINE_R, cA.y}), O({xTurn, cA.y}), !evenRow);
            DrawConveyorBelt(drawList, O({xTurn, cA.y}),                      O({xTurn, cB.y}));
            DrawConveyorBelt(drawList, O({xTurn, cB.y}), O({cB.x + rowDir * MACHINE_R, cB.y}), evenRow);
        } else {
            ImVec2 bStart = {cA.x + rowDir * MACHINE_R, cA.y};
            ImVec2 bEnd   = {cB.x - rowDir * MACHINE_R, cB.y};
            DrawConveyorBelt(drawList, O(bStart), O(bEnd), !evenRow);
        }
    }

    // Last machine → counter
    DrawConveyorBelt(drawList,
        O({centers[N-1].x + lastDir * MACHINE_R, centers[N-1].y}),
        O({counterCenter.x - lastDir * MACHINE_R, counterCenter.y}),
        lastDir < 0);

    // ── Draw pizzas on belts ─────────────────────────────────────────────────
    auto pizzaOnLine = [](int slot, int len, ImVec2 a, ImVec2 b) -> ImVec2 {
        float t = (slot + 0.5f) / len;
        return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
    };
    auto pizzaOnUturn = [&](int slot, int len, ImVec2 cA, ImVec2 cB, int dir) -> ImVec2 {
        float t      = (slot + 0.5f) / len;
        float xTurn  = cA.x + dir * (MACHINE_R + UTURN_DX);
        float seg1   = UTURN_DX, seg2 = fabsf(cB.y - cA.y), seg3 = UTURN_DX;
        float dist   = t * (seg1 + seg2 + seg3);
        if (dist < seg1)          return {cA.x + dir * (MACHINE_R + dist),       cA.y};
        if (dist < seg1 + seg2)   return {xTurn,                                  cA.y + (dist - seg1)};
        return {xTurn - dir * (dist - seg1 - seg2), cB.y};
    };

    for (int i = 0; i < N; i++) {
        if (!machList[i].nextBelt) continue;
        const auto& slots  = machList[i].nextBelt->getBelt();
        int         len    = (int)slots.size();
        int         row    = i / MACHINES_PER_ROW;
        bool        evenRow = (row % 2 == 0);
        int         rowDir  = evenRow ? 1 : -1;
        bool        lastInRow = ((i % MACHINES_PER_ROW) == MACHINES_PER_ROW - 1) && (i + 1 < N);
        bool        isLast    = (i == N - 1);

        for (int s = 0; s < len; s++) {
            if (!slots[s]) continue;
            ImVec2 pos;
            if (isLast) {
                pos = O(pizzaOnLine(s, len,
                    {centers[i].x + lastDir * MACHINE_R, centers[i].y},
                    {counterCenter.x - lastDir * MACHINE_R, counterCenter.y}));
            } else if (lastInRow) {
                pos = O(pizzaOnUturn(s, len, centers[i], centers[i + 1], rowDir));
            } else {
                pos = O(pizzaOnLine(s, len,
                    {centers[i].x + rowDir * MACHINE_R,     centers[i].y},
                    {centers[i+1].x - rowDir * MACHINE_R,   centers[i+1].y}));
            }
            DrawPizza(drawList, pos, slots[s]);
        }
    }

    // ── Draw spawner & counter ───────────────────────────────────────────────
    DrawDoughStorageVisual(drawList, O(spawnerCenter));
    DrawCounterVisual(drawList, O(counterCenter), m_model);

    // ── Draw machines ────────────────────────────────────────────────────────
    for (int i = 0; i < N; i++) {
        const char* nameKo = GetMachineNameKo(machList[i].machine);
        const char* icon   = GetMachineIcon(machList[i].machine);
        if (DrawMachineNode(drawList, O(centers[i]), machList[i].machine, nameKo, icon, machList[i].pipeIdx)) {
            if (m_selectedMachineIdx == machList[i].pipeIdx) {
                m_selectedMachineIdx = -1;
            } else {
                m_selectedMachineIdx = machList[i].pipeIdx;
                m_settingsPanelPos   = ImVec2(O(centers[i]).x + 52, O(centers[i]).y - 50);
            }
        }
    }

    drawList->AddText(ImGui::GetFont(), 13.0f, ImVec2(origin.x + 15, origin.y + canvasH - 20),
                      ImColor(150, 160, 170), "💡 좌클릭: 기계 설정 | 우클릭: 전원 ON/OFF");

    ImGui::EndChild();

    // 머신 설정 패널 (선택된 머신이 있을 때만)
    RenderMachineSettingsPanel();

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

void DashboardView::RenderMachineSettingsPanel()
{
    if (m_selectedMachineIdx < 0) return;

    const auto& pipeline = m_model->getPipeline();
    if (m_selectedMachineIdx >= (int)pipeline.size()) return;

    Machine* machine = pipeline[m_selectedMachineIdx];
    if (!machine) return;

    // 머신 인덱스로 이름 찾기
    const char* nameKo = GetMachineNameKo(machine);

    ImGui::SetNextWindowPos(m_settingsPanelPos, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(200, 0), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.95f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar
                           | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.88f, 0.88f, 0.90f, 0.97f));

    if (ImGui::Begin("##MachineSettings", nullptr, flags)) {

        // 타이틀 버튼
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.78f, 0.78f, 0.82f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.78f, 0.82f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.78f, 0.78f, 0.82f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::SetNextItemWidth(-1);
        ImGui::Button("머신 설정", ImVec2(-1, 0));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::Spacing();

        // 듀라빌리티 라벨 + 바
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

        ImGui::Button("듀라빌리티", ImVec2(-1, 0));
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        float durPct = machine->getDurability() / machine->getMaxDurability();
        if (durPct < 0.0f) durPct = 0.0f;
        if (durPct > 1.0f) durPct = 1.0f;
        ImVec4 barColor = (durPct > 0.6f) ? ImVec4(0.15f, 0.85f, 0.25f, 1.0f)
                        : (durPct > 0.3f) ? ImVec4(0.95f, 0.75f, 0.05f, 1.0f)
                                          : ImVec4(0.9f, 0.15f, 0.15f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
        ImGui::ProgressBar(durPct, ImVec2(-1, 10), "");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Speed 설정
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
            ImGui::BeginChild("##speedBox", ImVec2(-1, 60), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::Text("SPEED");
            float spd = machine->getSpeed();
            ImGui::SetNextItemWidth(80);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            if (ImGui::InputFloat("##spd", &spd, 0.5f, 1.0f, "%.1f")) {
                if (spd < 0.1f) spd = 0.1f;
                if (spd > 10.0f) spd = 10.0f;
                machine->setSpeed(spd);
            }
            ImGui::PopStyleColor(3);
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }

        ImGui::Spacing();

        // Capacity / Length 설정
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
            ImGui::BeginChild("##capBox", ImVec2(-1, 60), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

            if (auto* ncm = dynamic_cast<NonConveyorMachine*>(machine)) {
                ImGui::Text("CAPACITY");
                int cap = ncm->getCapacity();
                ImGui::SetNextItemWidth(80);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                if (ImGui::InputInt("##cap", &cap)) {
                    if (cap < 1) cap = 1;
                    if (cap > 8) cap = 8;
                    ncm->setCapacity(cap);
                }
                ImGui::PopStyleColor();
            } else if (auto* cm = dynamic_cast<ConveyorMachine*>(machine)) {
                ImGui::Text("LENGTH");
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                ImGui::Text("%d slots", cm->getLength());
                ImGui::PopStyleColor();
            }

            ImGui::PopStyleColor(2);
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }

        // 머신별 추가 설정
        if (auto* stretcher = dynamic_cast<DoughStretcher*>(machine)) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::Text("도우 크기:");
            int sz = (int)stretcher->getTargetSize();
            if (ImGui::RadioButton("S##sz", sz == 0)) stretcher->setTargetSize(PizzaSize::SMALL);
            ImGui::SameLine();
            if (ImGui::RadioButton("M##sz", sz == 1)) stretcher->setTargetSize(PizzaSize::MEDIUM);
            ImGui::SameLine();
            if (ImGui::RadioButton("L##sz", sz == 2)) stretcher->setTargetSize(PizzaSize::LARGE);
            ImGui::PopStyleColor();
        } else if (auto* oven = dynamic_cast<Oven*>(machine)) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            float temp = oven->getTemperature();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##temp", &temp, 150.0f, 350.0f, "%.0f°C")) {
                oven->setTemperature(temp);
            }
            ImGui::PopStyleColor();
        } else if (auto* cutter = dynamic_cast<Cutter*>(machine)) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            int slices = cutter->getSliceCount();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##slices", &slices, 4, 12, "%d 조각")) {
                if (slices % 2 != 0) slices++;
                cutter->setSliceCount(slices);
            }
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 브레이크 다운 버튼
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.85f, 0.85f, 0.88f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.70f, 0.70f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("브레이크 다운", ImVec2(-1, 0))) {
            m_controller->forceBreakMachine(m_selectedMachineIdx);
        }
        ImGui::Spacing();
        // 즉시 리페어 버튼
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.90f, 0.70f, 1.0f));
        if (ImGui::Button("즉시 리페어", ImVec2(-1, 0))) {
            m_controller->instantRepairMachine(m_selectedMachineIdx);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}
