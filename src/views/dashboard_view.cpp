#include "dashboard_view.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

// =============================================================================
//  bridge.h(값 구조체)만으로 그린다. 머신/피자 객체 포인터는 어디에도 없다.
//  사용 폰트(main.cpp)가 커버하는 글리프만 사용: ASCII + 한글 + BMP 기호
//  (▶ ⏸ ↻ ⚠ ● ▰ → ⌛ ✓ ✗). 0x1F000+ 이모지는 폰트 미포함이라 쓰지 않는다.
// =============================================================================

namespace {

// ── 상태 → 색/라벨 ───────────────────────────────────────────────────────────
ImVec4 StateColor(MachineState s) {
    switch (s) {
        case MachineState::WORKING: return ImVec4(0.26f, 0.85f, 0.42f, 1.0f);
        case MachineState::BROKEN:  return ImVec4(0.95f, 0.30f, 0.25f, 1.0f);
        case MachineState::OFF:     return ImVec4(0.50f, 0.52f, 0.56f, 1.0f);
        case MachineState::IDLE:
        default:                    return ImVec4(0.40f, 0.62f, 0.85f, 1.0f);
    }
}
const char* StateLabel(MachineState s) {
    switch (s) {
        case MachineState::WORKING: return "Working";
        case MachineState::BROKEN:  return "Broken";
        case MachineState::OFF:     return "Off";
        case MachineState::IDLE:
        default:                    return "Idle";
    }
}

// ── 피자 그리기 (PizzaView 값 구조체 기반) ───────────────────────────────────
void DrawPizza(ImDrawList* dl, ImVec2 c, const PizzaView& p) {
    float r = 15.0f;
    if (p.size == 0)      r = 11.0f;   // S
    else if (p.size == 2) r = 19.0f;   // L

    // 도우 단계별 색
    ImU32 dough = IM_COL32(245, 222, 179, 255);          // RAW 크림
    if (p.doughStage == 1) dough = IM_COL32(255, 239, 213, 255); // STRETCHED
    else if (p.doughStage == 2) dough = IM_COL32(210, 150, 75, 255); // BAKED

    dl->AddCircleFilled(c, r, dough);
    dl->AddCircle(c, r, IM_COL32(139, 90, 43, 255), 0, 1.5f);

    if (p.sauce)  dl->AddCircleFilled(c, r - 3.0f, IM_COL32(200, 40, 40, 255));
    if (p.cheese) dl->AddCircleFilled(c, r - 5.0f, IM_COL32(255, 220, 100, 220));
    if (p.hasTopping) {
        float d = r - 7.0f;
        if (d > 3.0f) {
            ImU32 pep = IM_COL32(150, 20, 20, 255);
            dl->AddCircleFilled(ImVec2(c.x - d/2, c.y - d/2), 2.5f, pep);
            dl->AddCircleFilled(ImVec2(c.x + d/2, c.y - d/2), 2.5f, pep);
            dl->AddCircleFilled(ImVec2(c.x - d/2, c.y + d/2), 2.5f, pep);
            dl->AddCircleFilled(ImVec2(c.x + d/2, c.y + d/2), 2.5f, pep);
            dl->AddCircleFilled(c, 2.5f, pep);
        }
    }
    if (p.cut) {
        const int count = 8;
        for (int i = 0; i < count / 2; ++i) {
            float a = i * (3.14159265f / (count / 2));
            ImVec2 d(std::cos(a) * r, std::sin(a) * r);
            dl->AddLine(ImVec2(c.x - d.x, c.y - d.y), ImVec2(c.x + d.x, c.y + d.y),
                        IM_COL32(60, 35, 12, 150), 1.0f);
        }
    }
    if (p.boxed) {
        dl->AddRect(ImVec2(c.x - r - 4, c.y - r - 4), ImVec2(c.x + r + 4, c.y + r + 4),
                    IM_COL32(180, 130, 90, 255), 4.0f, 0, 2.0f);
    }
}

// ── 컨베이어 벨트 스트립 (시간 기반 슬릿 애니메이션) ─────────────────────────
void DrawBelt(ImDrawList* dl, ImVec2 a, ImVec2 b, bool reverse) {
    dl->AddLine(a, b, IM_COL32(62, 67, 73, 255), 26.0f);
    ImVec2 dir(b.x - a.x, b.y - a.y);
    float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (len <= 0.0f) return;
    dir.x /= len; dir.y /= len;
    ImVec2 n(-dir.y, dir.x);
    dl->AddLine(ImVec2(a.x + n.x*13, a.y + n.y*13), ImVec2(b.x + n.x*13, b.y + n.y*13), IM_COL32(38,40,44,255), 2.0f);
    dl->AddLine(ImVec2(a.x - n.x*13, a.y - n.y*13), ImVec2(b.x - n.x*13, b.y - n.y*13), IM_COL32(38,40,44,255), 2.0f);
    float off = std::fmod((float)ImGui::GetTime() * 26.0f, 14.0f);
    if (reverse) off = 14.0f - off;
    for (float d = off; d < len; d += 14.0f) {
        ImVec2 m(a.x + dir.x*d, a.y + dir.y*d);
        dl->AddLine(ImVec2(m.x - n.x*9, m.y - n.y*9), ImVec2(m.x + n.x*9, m.y + n.y*9), IM_COL32(48,53,58,255), 2.0f);
    }
}

// ── 얇은 진행 바 ─────────────────────────────────────────────────────────────
void DrawBar(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float pct, ImU32 col) {
    pct = pct < 0 ? 0 : (pct > 1 ? 1 : pct);
    dl->AddRectFilled(mn, mx, IM_COL32(35, 38, 42, 255), 2.0f);
    if (pct > 0)
        dl->AddRectFilled(mn, ImVec2(mn.x + (mx.x - mn.x) * pct, mx.y), col, 2.0f);
}

float HealthHue(float pct, ImU32& out) {
    if (pct < 0.3f)      out = IM_COL32(231, 76, 60, 255);
    else if (pct < 0.8f) out = IM_COL32(241, 196, 15, 255);
    else                 out = IM_COL32(46, 204, 113, 255);
    return pct;
}

} // namespace

// =============================================================================
//  창 배치 (최초 1회) — 1280x720 기준 타일링
// =============================================================================
void DashboardView::Render(const FactorySnap& snap, FactoryCmd& cmd)
{
    // 매 프레임 "지속 의도"를 cmd 에 싣는다 (한 프레임 플래그는 각 버튼에서 set).
    // selectedMachine 은 RenderFloor 에서 선택이 확정된 뒤 실어 같은 프레임에 반영.
    cmd.speed = m_speedUI;

    if (m_firstFrame) m_speedUI = snap.speed > 0 ? snap.speed : 1;

    RenderControl(snap, cmd);
    RenderFloor(snap, cmd);
    RenderInspector(snap, cmd);
    RenderEventLog(snap, cmd);
    RenderStatistics(snap, cmd);
    RenderOrders(snap, cmd);

    m_firstFrame = false;
}

// =============================================================================
//  1) Simulation Control
// =============================================================================
void DashboardView::RenderControl(const FactorySnap& snap, FactoryCmd& cmd)
{
    ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(860, 96), ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation Control");

    // ── Start / Pause 토글 ──
    if (snap.running) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.75f, 0.16f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.22f, 0.22f, 1.0f));
        if (ImGui::Button(u8"⏸ Pause", ImVec2(120, 0))) cmd.pause = true;
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.62f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.78f, 0.32f, 1.0f));
        if (ImGui::Button(u8"▶ Start", ImVec2(120, 0))) cmd.start = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    if (ImGui::Button(u8"↻ Reset", ImVec2(90, 0))) cmd.reset = true;

    // ── 배속 SliderInt (1x–5x) ──
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    if (ImGui::SliderInt("Speed", &m_speedUI, 1, 5, "%dx")) cmd.speed = m_speedUI;

    // ── 시나리오 Combo ──
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    std::vector<const char*> names;
    names.reserve(snap.scenarioNames.size());
    for (const auto& s : snap.scenarioNames) names.push_back(s.c_str());
    int cur = snap.scenario;
    if (!names.empty() &&
        ImGui::Combo("Scenario", &cur, names.data(), (int)names.size()))
        cmd.scenario = cur;

    // ── 틱 카운터 + 잔고 ──
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.78f, 0.85f, 1.0f), "Tick %ld", snap.tick);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "$ %d", snap.money);

    ImGui::End();
}

// =============================================================================
//  2) Factory Floor — 스네이크 파이프라인 캔버스 + Selectable 머신 리스트
// =============================================================================
void DashboardView::RenderFloor(const FactorySnap& snap, FactoryCmd& cmd)
{
    ImGui::SetNextWindowPos(ImVec2(8, 112), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(860, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Factory Floor");

    const auto& M = snap.machines;
    const int   N = (int)M.size();

    // ── 레이아웃 상수 ──
    const float marginX = 56.0f, marginTop = 36.0f, rowH = 130.0f, gap = 16.0f;
    const float nodeR   = 38.0f, slotW = 30.0f;

    float canvasW = ImGui::GetContentRegionAvail().x;
    if (canvasW < 360.0f) canvasW = 360.0f;

    // ── 아이템(머신) 폭 계산 ──
    struct Item { int idx; bool conv; float w, cx, cy; int row, dir; int slots; };
    std::vector<Item> items;
    items.reserve(N);
    for (int i = 0; i < N; ++i) {
        const MachineSnap& m = M[i];
        int slots = m.isConveyor ? std::max(2, (int)m.conveyor.slots.size()) : 0;
        float w = m.isConveyor ? slots * slotW : nodeR * 2.0f;
        items.push_back({i, m.isConveyor, w, 0, 0, 0, 0, slots});
    }

    // ── 스네이크(보스트로페돈) 배치 ──
    int   row = 0, dir = 1, maxRow = 0;
    float x = marginX;
    const float rightLimit = canvasW - marginX;
    for (int i = 0; i < N; ++i) {
        float w = items[i].w;
        if (dir > 0) {
            if (i > 0 && x + w > rightLimit) { ++row; dir = -1; x = rightLimit; }
        } else {
            if (i > 0 && x - w < marginX)    { ++row; dir =  1; x = marginX; }
        }
        float cy = marginTop + nodeR + row * rowH;
        if (dir > 0) { items[i].cx = x + w*0.5f; x += w + gap; }
        else         { items[i].cx = x - w*0.5f; x -= w + gap; }
        items[i].cy = cy; items[i].row = row; items[i].dir = dir;
        maxRow = std::max(maxRow, row);
    }
    float canvasH = marginTop + (maxRow + 1) * rowH + 4.0f;

    // ── 캔버스 베이스 + 그리드 ──
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    auto O = [&](ImVec2 p) { return ImVec2(origin.x + p.x, origin.y + p.y); };

    dl->AddRectFilled(origin, ImVec2(origin.x + canvasW, origin.y + canvasH), IM_COL32(28, 32, 38, 255), 10.0f);
    for (float g = 36; g < canvasW; g += 36)
        dl->AddLine(ImVec2(origin.x + g, origin.y), ImVec2(origin.x + g, origin.y + canvasH), IM_COL32(40, 45, 50, 90));
    for (float g = 36; g < canvasH; g += 36)
        dl->AddLine(ImVec2(origin.x, origin.y + g), ImVec2(origin.x + canvasW, origin.y + g), IM_COL32(40, 45, 50, 90));

    auto entryPort = [&](const Item& it) {
        return it.dir > 0 ? ImVec2(it.cx - it.w*0.5f, it.cy) : ImVec2(it.cx + it.w*0.5f, it.cy);
    };
    auto exitPort = [&](const Item& it) {
        return it.dir > 0 ? ImVec2(it.cx + it.w*0.5f, it.cy) : ImVec2(it.cx - it.w*0.5f, it.cy);
    };

    // ── 머신 사이 커넥터(단순 링크: 비-벨트 이송은 즉시이므로 피자 없음) ──
    for (int i = 0; i + 1 < N; ++i) {
        ImVec2 a = exitPort(items[i]);
        ImVec2 b = entryPort(items[i + 1]);
        ImU32 link = IM_COL32(90, 98, 108, 255);
        if (items[i].row == items[i + 1].row) {
            dl->AddLine(O(a), O(b), link, 3.0f);
            ImVec2 mid((a.x + b.x) * 0.5f, a.y);                 // 화살표
            float s = (b.x > a.x) ? 1.0f : -1.0f;
            dl->AddTriangleFilled(O(ImVec2(mid.x + s*5, mid.y)), O(ImVec2(mid.x - s*4, mid.y - 4)),
                                  O(ImVec2(mid.x - s*4, mid.y + 4)), link);
        } else {                                                // U-turn: 수직 낙하
            dl->AddLine(O(a), O(ImVec2(b.x, b.y)), link, 3.0f);
        }
    }

    // ── 벨트 머신 + 그 위 피자 ──
    for (const Item& it : items) {
        if (!it.conv) continue;
        const MachineSnap& m = M[it.idx];
        ImVec2 en = entryPort(it), ex = exitPort(it);
        DrawBelt(dl, O(en), O(ex), it.dir < 0);
        if (m.state == MachineState::BROKEN)
            dl->AddLine(O(en), O(ex), IM_COL32(200, 40, 40, 90), 26.0f);
        int len = (int)m.conveyor.slots.size();
        for (int s = 0; s < len; ++s) {
            if (!m.conveyor.slots[s].occupied) continue;
            float u = (s + 0.5f + m.conveyor.moveProgress) / (float)len;
            if (u > 1.05f) u = 1.05f;
            ImVec2 pos(en.x + (ex.x - en.x) * u, en.y + (ex.y - en.y) * u);
            DrawPizza(dl, O(pos), m.conveyor.slots[s].pizza);
        }
    }

    // ── 비-벨트 머신 노드 ──
    auto drawNode = [&](const Item& it) {
        const MachineSnap& m = M[it.idx];
        ImVec2 c = O(ImVec2(it.cx, it.cy));
        ImVec2 mn(c.x - nodeR, c.y - nodeR), mx(c.x + nodeR, c.y + nodeR);

        ImU32 bg = IM_COL32(38, 48, 58, 255), bd = IM_COL32(110, 120, 130, 255);
        float bw = 2.0f;
        switch (m.state) {
            case MachineState::WORKING: bd = IM_COL32(46, 204, 113, 255); break;
            case MachineState::IDLE:    bd = IM_COL32(52, 152, 219, 255); break;
            case MachineState::OFF:     bg = IM_COL32(26, 30, 34, 255); bd = IM_COL32(70, 75, 80, 255); break;
            case MachineState::BROKEN: {
                float pulse = 0.5f + 0.5f * std::sin((float)ImGui::GetTime() * 11.0f);
                bd = IM_COL32(255, (int)(40 + 70*pulse), (int)(40 + 70*pulse), 255); bw = 3.0f;
            } break;
        }
        dl->AddRectFilled(mn, mx, bg, 11.0f);
        dl->AddRect(mn, mx, bd, 11.0f, 0, bw);
        if (it.idx == m_selected)
            dl->AddRect(ImVec2(mn.x-3, mn.y-3), ImVec2(mx.x+3, mx.y+3), IM_COL32(255, 214, 64, 255), 13.0f, 0, 2.5f);

        // 이름 (노드 위)
        ImVec2 ts = ImGui::CalcTextSize(m.name.c_str());
        dl->AddText(ImVec2(c.x - ts.x*0.5f, c.y - nodeR - 17),
                    m.state == MachineState::OFF ? IM_COL32(120,125,130,255) : IM_COL32(225,235,245,255),
                    m.name.c_str());

        // 내용물 피자 / 고장 경고
        if (m.state == MachineState::BROKEN) {
            ImVec2 ws = ImGui::CalcTextSize(u8"⚠");
            dl->AddText(ImGui::GetFont(), 26.0f, ImVec2(c.x - ws.x*0.7f, c.y - 20), IM_COL32(255,70,70,255), u8"⚠");
        } else if (m.hasPizzaInside) {
            DrawPizza(dl, ImVec2(c.x, c.y - 4), m.pizzaInside);
        } else {
            dl->AddCircle(ImVec2(c.x, c.y - 4), 13.0f, IM_COL32(70, 78, 88, 180), 0, 1.5f);
        }

        // 진행 바 + 체력 바
        ImU32 hcol; HealthHue(m.healthPct, hcol);
        DrawBar(dl, ImVec2(c.x-30, c.y+nodeR-15), ImVec2(c.x+30, c.y+nodeR-11), m.progressPct, IM_COL32(80, 170, 240, 255));
        DrawBar(dl, ImVec2(c.x-30, c.y+nodeR-8),  ImVec2(c.x+30, c.y+nodeR-4),  m.healthPct,   hcol);
    };
    for (const Item& it : items) if (!it.conv) drawNode(it);

    // ── 입고 / 출고 표식 ──
    if (N > 0) {
        ImVec2 in = O(entryPort(items[0]));
        dl->AddText(ImVec2(in.x - 50, in.y - 8), IM_COL32(150, 200, 150, 255), u8"IN ▸");
        ImVec2 out = O(exitPort(items[N-1]));
        char buf[48]; std::snprintf(buf, sizeof(buf), u8"▸ OUT %d", snap.finishedGoods);
        dl->AddText(ImVec2(out.x + 6, out.y - 8), IM_COL32(255, 210, 120, 255), buf);
    }

    // ── 클릭 영역(InvisibleButton) → 선택 토글 ──
    for (const Item& it : items) {
        ImGui::SetCursorScreenPos(ImVec2(O(ImVec2(it.cx, it.cy)).x - it.w*0.5f,
                                         O(ImVec2(it.cx, it.cy)).y - nodeR));
        ImGui::PushID(it.idx);
        if (ImGui::InvisibleButton("##node", ImVec2(it.w, nodeR * 2.0f)))
            m_selected = (m_selected == it.idx) ? -1 : it.idx;
        ImGui::PopID();
    }

    // 캔버스만큼 커서 전진
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy(ImVec2(canvasW, canvasH));

    // ── 머신 목록: Selectable + 상태색 + 부하/진행 ProgressBar ──
    ImGui::Spacing();
    ImGui::TextDisabled("Machines  (click a row to select)");
    for (int i = 0; i < N; ++i) {
        const MachineSnap& m = M[i];
        ImGui::PushID(1000 + i);
        bool sel = (i == m_selected);
        if (ImGui::Selectable("##row", sel, 0, ImVec2(0, 22)))
            m_selected = sel ? -1 : i;
        ImGui::SameLine(8);  ImGui::TextUnformatted(m.name.c_str());
        ImGui::SameLine(150); ImGui::TextColored(StateColor(m.state), "%s", StateLabel(m.state));
        ImGui::SameLine(210);
        float frac; char ov[32];
        if (m.isConveyor) {
            int cap = std::max(1, (int)m.conveyor.slots.size());
            frac = (float)m.queueDepth / cap;
            std::snprintf(ov, sizeof(ov), "load %d/%d", m.queueDepth, cap);
        } else {
            frac = m.progressPct;
            std::snprintf(ov, sizeof(ov), "%d%%", (int)(m.progressPct * 100));
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::ProgressBar(frac, ImVec2(-1, 16), ov);
        ImGui::PopID();
    }

    // 선택 확정 후 cmd 에 실어 Force Break / Instant Repair 대상이 되게 한다.
    cmd.selectedMachine = m_selected;

    ImGui::End();
}

// =============================================================================
//  3) Inspector
// =============================================================================
void DashboardView::RenderInspector(const FactorySnap& snap, FactoryCmd& cmd)
{
    ImGui::SetNextWindowPos(ImVec2(8, 520), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(425, 192), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector");

    if (m_selected < 0 || m_selected >= (int)snap.machines.size()) {
        ImGui::TextDisabled("Select a machine.");
        ImGui::TextDisabled("(click a node or list row in Factory Floor)");
        ImGui::End();
        return;
    }

    const MachineSnap& m = snap.machines[m_selected];
    ImGui::Text("#%d  %s", m.id, m.name.c_str());
    ImGui::SameLine();
    ImGui::TextColored(StateColor(m.state), "[%s]", StateLabel(m.state));
    ImGui::Separator();

    // 체력 바
    ImU32 hc; HealthHue(m.healthPct, hc);
    ImGui::Text("Health");
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImColor(hc).Value);
    ImGui::ProgressBar(m.healthPct, ImVec2(-1, 14));
    ImGui::PopStyleColor();

    // 진행 바
    ImGui::Text("Progress");
    ImGui::ProgressBar(m.progressPct, ImVec2(-1, 14));

    ImGui::Text("queue depth : %d", m.queueDepth);
    ImGui::Text("output count: %d", m.outputCount);
    ImGui::Text("process time: %d ticks", m.processTicks);

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.62f, 0.20f, 0.18f, 1.0f));
    if (ImGui::Button("Force Break", ImVec2(150, 0))) cmd.forceBreak = true;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.50f, 0.30f, 1.0f));
    if (ImGui::Button("Instant Repair", ImVec2(150, 0))) cmd.instantRepair = true;
    ImGui::PopStyleColor();

    ImGui::End();
}

// =============================================================================
//  4) Event Log
// =============================================================================
void DashboardView::RenderEventLog(const FactorySnap& snap, FactoryCmd& cmd)
{
    ImGui::SetNextWindowPos(ImVec2(441, 520), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(427, 192), ImGuiCond_FirstUseEver);
    ImGui::Begin("Event Log");

    if (ImGui::Button("Clear")) cmd.clearLog = true;
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    ImGui::SameLine();
    ImGui::TextDisabled("(%d)", (int)snap.eventLog.size());
    ImGui::Separator();

    ImGui::BeginChild("logScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& line : snap.eventLog)
        ImGui::TextUnformatted(line.c_str());
    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End();
}

// =============================================================================
//  5) Statistics
// =============================================================================
void DashboardView::RenderStatistics(const FactorySnap& snap, FactoryCmd& cmd)
{
    (void)cmd;
    ImGui::SetNextWindowPos(ImVec2(876, 8), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(396, 190), ImGuiCond_FirstUseEver);
    ImGui::Begin("Statistics");

    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.45f, 1.0f), "finished goods : %d", snap.finishedGoods);
    ImGui::TextColored(ImVec4(0.5f, 0.75f, 1.0f, 1.0f),  "WIP count      : %d", snap.wipCount);
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f),   "breakdowns     : %d", snap.totalBreakdowns);
    ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.3f, 1.0f), "lost products  : %d", snap.lostProducts);
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),  "earnings       : $ %d", snap.money);
    ImGui::Text("active orders  : %d", (int)snap.orders.size());
    ImGui::Text("tick           : %ld  (x%d)", snap.tick, snap.speed);

    ImGui::End();
}

// =============================================================================
//  6) Orders (게임화 보너스)
// =============================================================================
void DashboardView::RenderOrders(const FactorySnap& snap, FactoryCmd& cmd)
{
    (void)cmd;
    ImGui::SetNextWindowPos(ImVec2(876, 206), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(396, 506), ImGuiCond_FirstUseEver);
    ImGui::Begin("Orders");

    if (snap.orders.empty()) {
        ImGui::TextDisabled("No active orders.");
        ImGui::TextDisabled("Orders arrive once you press Start.");
        ImGui::End();
        return;
    }

    for (const OrderSnap& o : snap.orders) {
        ImGui::PushID(o.id);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.17f, 0.20f, 1.0f));
        ImGui::BeginChild("slip", ImVec2(0, 64), true, ImGuiWindowFlags_NoScrollbar);

        ImGui::TextUnformatted(o.desc.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "$%d", o.reward);

        float frac = o.ticksLeft / 600.0f;       // 생성 상한(600틱) 기준 정규화
        if (frac > 1.0f) frac = 1.0f; if (frac < 0.0f) frac = 0.0f;
        ImU32 bc = (frac > 0.6f) ? IM_COL32(46,204,113,255)
                 : (frac > 0.3f) ? IM_COL32(241,196,15,255)
                                 : IM_COL32(231,76,60,255);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImColor(bc).Value);
        char ov[32]; std::snprintf(ov, sizeof(ov), u8"⌛ %d", o.ticksLeft);
        ImGui::ProgressBar(frac, ImVec2(-1, 14), ov);
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::Spacing();
    }

    ImGui::End();
}
