#include "dashboard_view.h"
#include "belt_render.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

// =============================================================================
//  bridge.h(값 구조체)만으로 그린다. 머신/피자 객체 포인터는 어디에도 없다.
//  사용 폰트(main.cpp)가 커버하는 글리프만 사용: ASCII + 한글 + BMP 기호
//  (▶ ⏸ ↻ ⚠ ● ▰ → ⌛). 0x1F000+ 이모지는 폰트 미포함이라 쓰지 않는다.
// =============================================================================

namespace {

constexpr float kPi = 3.14159265f;

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

    ImU32 dough = IM_COL32(245, 222, 179, 255);              // RAW
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
            float a = i * (kPi / (count / 2));
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

// 컨베이어 벨트 드로잉 프리미티브(DrawBelt / DrawBeltArc)는 belt_render.h 로 분리.

void DrawBar(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float pct, ImU32 col) {
    pct = pct < 0 ? 0 : (pct > 1 ? 1 : pct);
    dl->AddRectFilled(mn, mx, IM_COL32(35, 38, 42, 255), 2.0f);
    if (pct > 0)
        dl->AddRectFilled(mn, ImVec2(mn.x + (mx.x - mn.x) * pct, mx.y), col, 2.0f);
}

void HealthHue(float pct, ImU32& out) {
    if (pct < 0.3f)      out = IM_COL32(231, 76, 60, 255);
    else if (pct < 0.8f) out = IM_COL32(241, 196, 15, 255);
    else                 out = IM_COL32(46, 204, 113, 255);
}

} // namespace

// =============================================================================
void DashboardView::Render(const FactorySnap& snap, FactoryCmd& cmd)
{
    cmd.speed           = m_speedUI;
    cmd.selectedMachine = m_selected;
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

    if (snap.running) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.75f, 0.16f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.22f, 0.22f, 1.0f));
        if (ImGui::Button("Pause", ImVec2(120, 0))) cmd.pause = true;
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.62f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.78f, 0.32f, 1.0f));
        if (ImGui::Button("Start", ImVec2(120, 0))) cmd.start = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(90, 0))) cmd.reset = true;

    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    if (ImGui::SliderInt("Speed", &m_speedUI, 1, 5, "%dx")) cmd.speed = m_speedUI;

    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    std::vector<const char*> names;
    names.reserve(snap.scenarioNames.size());
    for (const auto& s : snap.scenarioNames) names.push_back(s.c_str());
    int cur = snap.scenario;
    if (!names.empty() &&
        ImGui::Combo("Scenario", &cur, names.data(), (int)names.size()))
        cmd.scenario = cur;

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.78f, 0.85f, 1.0f), "Tick %ld", snap.tick);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "$ %d", snap.money);

    ImGui::End();
}

// =============================================================================
//  2) Factory Floor — station 스네이크 + 직선/반원 벨트 캔버스
// =============================================================================
void DashboardView::RenderFloor(const FactorySnap& snap, FactoryCmd& cmd)
{
    (void)cmd;
    ImGui::SetNextWindowPos(ImVec2(8, 112), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(860, 364), ImGuiCond_FirstUseEver);
    ImGui::Begin("Factory Floor", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

    const auto& M = snap.machines;
    const int   N = (int)M.size();

    // 비-벨트 머신(station) 의 파이프라인 인덱스
    std::vector<int> st;
    for (int i = 0; i < N; ++i) if (!M[i].isConveyor) st.push_back(i);
    const int S = (int)st.size();

    // 레이아웃 상수
    const int   perRow  = 4;
    const float nodeR   = 34.0f, cellW = 152.0f;
    const float marginX = 72.0f, marginTop = 50.0f, rowH = 122.0f;
    const float arcR    = rowH * 0.5f;
    const float rightmost = marginX + nodeR + (perRow - 1) * cellW;

    auto stCenter = [&](int si) -> ImVec2 {
        int row = si / perRow, col = si % perRow;
        bool ltr = (row % 2 == 0);
        float cx = ltr ? (marginX + nodeR + col * cellW) : (rightmost - col * cellW);
        float cy = marginTop + nodeR + row * rowH;
        return ImVec2(cx, cy);
    };
    auto stDir = [&](int si) { return ((si / perRow) % 2 == 0) ? 1 : -1; };  // +1 LTR, -1 RTL

    int   rows    = (S + perRow - 1) / perRow;
    float canvasW = rightmost + nodeR + arcR + 26.0f;
    float availW  = ImGui::GetContentRegionAvail().x;
    if (canvasW < availW) canvasW = availW;
    float canvasH = marginTop + rows * rowH + 6.0f;

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    auto O = [&](ImVec2 p) { return ImVec2(origin.x + p.x, origin.y + p.y); };

    // 배경 + 그리드
    dl->AddRectFilled(origin, ImVec2(origin.x + canvasW, origin.y + canvasH), IM_COL32(28,32,38,255), 10.0f);
    for (float g = 36; g < canvasW; g += 36)
        dl->AddLine(ImVec2(origin.x+g, origin.y), ImVec2(origin.x+g, origin.y+canvasH), IM_COL32(40,45,50,80));
    for (float g = 36; g < canvasH; g += 36)
        dl->AddLine(ImVec2(origin.x, origin.y+g), ImVec2(origin.x+canvasW, origin.y+g), IM_COL32(40,45,50,80));

    // ── 벨트(station 사이) ──
    struct BeltHit { int idx; ImVec2 mn, mx; };   // 클릭 영역 (벨트도 선택 가능)
    std::vector<BeltHit> beltHits;
    const ImU32 selCol = IM_COL32(255, 214, 64, 255);
    for (int si = 0; si + 1 < S; ++si) {
        int  beltIdx = st[si] + 1;
        bool hasBelt = (beltIdx < st[si+1]) && M[beltIdx].isConveyor;
        const MachineSnap* belt = hasBelt ? &M[beltIdx] : nullptr;
        int   len = belt ? (int)belt->conveyor.slots.size() : 0;
        float mp  = belt ? belt->conveyor.moveProgress : 0.0f;
        bool  brk = belt && belt->state == MachineState::BROKEN;
        ImVec2 A = stCenter(si), B = stCenter(si + 1);
        int    dA = stDir(si);
        bool sameRow = (si / perRow) == ((si + 1) / perRow);

        if (sameRow) {
            ImVec2 a = O(ImVec2(A.x + dA*nodeR, A.y));
            ImVec2 b = O(ImVec2(B.x - dA*nodeR, B.y));
            belt::DrawBelt(dl, a, b, false);  // a,b가 이미 흐름 방향 → 줄무늬는 항상 a→b
            if (brk) dl->AddLine(a, b, IM_COL32(200,40,40,90), 26.0f);
            if (hasBelt) {
                ImVec2 mn(std::min(a.x, b.x), a.y - 16), mx(std::max(a.x, b.x), a.y + 16);
                beltHits.push_back({beltIdx, mn, mx});
                if (beltIdx == m_selected) dl->AddRect(mn, mx, selCol, 8.0f, 0, 2.5f);
            }
            for (int s = 0; s < len; ++s) {
                if (!belt->conveyor.slots[s].occupied) continue;
                float t = (s + 0.5f + mp) / len; if (t > 1.05f) t = 1.05f;
                DrawPizza(dl, ImVec2(a.x + (b.x-a.x)*t, a.y + (b.y-a.y)*t), belt->conveyor.slots[s].pizza);
            }
        } else {
            int   side  = (dA > 0) ? 1 : -1;                 // 우측 LTR / 좌측 RTL
            float edgeX = A.x + side * nodeR;
            float midY  = (A.y + B.y) * 0.5f;
            ImVec2 c    = O(ImVec2(edgeX, midY));
            float a0 = -kPi * 0.5f;                          // -90° (위, A)
            float a1 = (side > 0) ? (kPi * 0.5f)             // 우측 반원: +90°
                                  : (-kPi * 1.5f);           // 좌측 반원: -270°
            belt::DrawBeltArc(dl, c, arcR, a0, a1, dA < 0);
            if (brk) { dl->PathArcTo(c, arcR, a0, a1, 30); dl->PathStroke(IM_COL32(200,40,40,90), 0, 26.0f); }
            if (hasBelt) {
                float x1 = c.x + side * (arcR + 16.0f);
                ImVec2 mn(std::min(c.x, x1), c.y - arcR - 16), mx(std::max(c.x, x1), c.y + arcR + 16);
                beltHits.push_back({beltIdx, mn, mx});
                if (beltIdx == m_selected) {
                    dl->PathArcTo(c, arcR + 16.0f, a0, a1, 24); dl->PathStroke(selCol, 0, 2.5f);
                    dl->PathArcTo(c, arcR - 16.0f, a0, a1, 24); dl->PathStroke(selCol, 0, 2.5f);
                }
            }
            for (int s = 0; s < len; ++s) {
                if (!belt->conveyor.slots[s].occupied) continue;
                float t = (s + 0.5f + mp) / len; if (t > 1.05f) t = 1.05f;
                float g = a0 + (a1 - a0) * t;
                DrawPizza(dl, ImVec2(c.x + arcR*std::cos(g), c.y + arcR*std::sin(g)),
                          belt->conveyor.slots[s].pizza);
            }
        }
    }

    // ── station 노드 ──
    auto drawNode = [&](int si) {
        const MachineSnap& m = M[st[si]];
        ImVec2 c = O(stCenter(si));
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
        if (st[si] == m_selected)
            dl->AddRect(ImVec2(mn.x-3, mn.y-3), ImVec2(mx.x+3, mx.y+3), IM_COL32(255, 214, 64, 255), 13.0f, 0, 2.5f);

        ImVec2 ts = ImGui::CalcTextSize(m.name.c_str());
        dl->AddText(ImVec2(c.x - ts.x*0.5f, c.y - nodeR - 16),
                    m.state == MachineState::OFF ? IM_COL32(120,125,130,255) : IM_COL32(225,235,245,255),
                    m.name.c_str());

        if (m.state == MachineState::BROKEN) {
            // 폰트 비의존 "고장" 표시: 빨간 X 를 선으로 직접 그린다.
            float k = 11.0f;
            dl->AddLine(ImVec2(c.x-k, c.y-3-k), ImVec2(c.x+k, c.y-3+k), IM_COL32(255,70,70,255), 3.0f);
            dl->AddLine(ImVec2(c.x-k, c.y-3+k), ImVec2(c.x+k, c.y-3-k), IM_COL32(255,70,70,255), 3.0f);
        } else if (m.hasPizzaInside) {
            DrawPizza(dl, ImVec2(c.x, c.y - 3), m.pizzaInside);
        } else {
            dl->AddCircle(ImVec2(c.x, c.y - 3), 12.0f, IM_COL32(70, 78, 88, 180), 0, 1.5f);
        }

        ImU32 hcol; HealthHue(m.healthPct, hcol);
        DrawBar(dl, ImVec2(c.x-28, c.y+nodeR-15), ImVec2(c.x+28, c.y+nodeR-11), m.progressPct, IM_COL32(80, 170, 240, 255));
        DrawBar(dl, ImVec2(c.x-28, c.y+nodeR-8),  ImVec2(c.x+28, c.y+nodeR-4),  m.healthPct,   hcol);
    };
    for (int si = 0; si < S; ++si) drawNode(si);

    // ── 입고 / 출고 표식 ──
    if (S > 0) {
        ImVec2 in = O(stCenter(0));
        dl->AddText(ImVec2(in.x - nodeR - 34, in.y - 8), IM_COL32(150, 200, 150, 255), "IN");
        ImVec2 oc = O(stCenter(S - 1));
        int dL = stDir(S - 1);
        char buf[48]; std::snprintf(buf, sizeof(buf), "OUT %d", snap.finishedGoods);
        float ox = (dL > 0) ? (oc.x + nodeR + 6) : (oc.x - nodeR - 64);
        dl->AddText(ImVec2(ox, oc.y - 8), IM_COL32(255, 210, 120, 255), buf);
    }

    // ── 클릭 영역(InvisibleButton) → 선택 토글 ──
    for (int si = 0; si < S; ++si) {
        ImVec2 c = O(stCenter(si));
        ImGui::SetCursorScreenPos(ImVec2(c.x - nodeR, c.y - nodeR));
        ImGui::PushID(st[si]);
        if (ImGui::InvisibleButton("##node", ImVec2(nodeR*2, nodeR*2)))
            m_selected = (m_selected == st[si]) ? -1 : st[si];
        ImGui::PopID();
    }
    for (const BeltHit& bh : beltHits) {
        ImGui::SetCursorScreenPos(bh.mn);
        ImGui::PushID(2000 + bh.idx);
        if (ImGui::InvisibleButton("##belt", ImVec2(bh.mx.x - bh.mn.x, bh.mx.y - bh.mn.y)))
            m_selected = (m_selected == bh.idx) ? -1 : bh.idx;
        ImGui::PopID();
    }

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
        ImGui::SameLine(8);   ImGui::TextUnformatted(m.name.c_str());
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

    ImGui::End();
}

// =============================================================================
//  3) Inspector
// =============================================================================
void DashboardView::RenderInspector(const FactorySnap& snap, FactoryCmd& cmd)
{
    ImGui::SetNextWindowPos(ImVec2(8, 484), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(425, 228), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector");

    if (m_selected < 0 || m_selected >= (int)snap.machines.size()) {
        ImGui::TextDisabled("Select a machine.");
        ImGui::TextDisabled("(click a node or belt in Factory Floor)");
        ImGui::End();
        return;
    }

    const MachineSnap& m = snap.machines[m_selected];
    ImGui::Text("#%d  %s", m.id, m.name.c_str());
    ImGui::SameLine();
    ImGui::TextColored(StateColor(m.state), "[%s]", StateLabel(m.state));
    ImGui::Separator();

    // ── 상태 (읽기 전용) ──
    char ov[40];
    // ImGui::Dummy(ImVec2(0, 4)); // very little blank space
    if (m.isConveyor) {
        int cap = std::max(1, (int)m.conveyor.slots.size());
        std::snprintf(ov, sizeof(ov), "load %d/%d", m.queueDepth, cap);
        ImGui::ProgressBar((float)m.queueDepth / cap, ImVec2(-1, 20), ov);
    } else {
        std::snprintf(ov, sizeof(ov), "progress %d%%", (int)(m.progressPct * 100));
        ImGui::ProgressBar(m.progressPct, ImVec2(-1, 20), ov);
    }
    // ImGui::Dummy(ImVec2(0, 8)); // very little blank space
    ImGui::Text("queue %d    output %d", m.queueDepth, m.outputCount);

    // ── 설정 (조절 → cmd.tune, 다음 틱에 반영) ──
    ImGui::Separator();
    ImGui::TextDisabled("Settings (applied live)");

    float healthUI = m.healthPct * 100.0f;
    if (ImGui::SliderFloat("Health", &healthUI, 0.0f, 100.0f, "%.0f%%"))
        cmd.tune.healthPct = healthUI / 100.0f;

    if (m.isConveyor) {
        float bs = m.beltSpeed;
        if (ImGui::SliderFloat("Belt speed", &bs, 0.05f, 1.0f, "%.2f slot/tick"))
            cmd.tune.beltSpeed = bs;
    } else {
        int pt = m.processTicks;
        if (ImGui::SliderInt("Proc time", &pt, 1, 20, "%d ticks"))
            cmd.tune.processTicks = pt;
    }

    float bp = m.breakProb * 100.0f;
    if (ImGui::SliderFloat("Break odds", &bp, 0.0f, 5.0f, "%.2f%%/tick"))
        cmd.tune.breakProb = bp / 100.0f;

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
    ImGui::SetNextWindowPos(ImVec2(441, 484), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(427, 228), ImGuiCond_FirstUseEver);
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

    if (!snap.ordersEnabled) {
        ImGui::TextDisabled("Orders are off in this scenario.");
        ImGui::TextDisabled("(production demo - pick Free Play to play)");
        ImGui::End();
        return;
    }

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

        float frac = o.ticksLeft / 600.0f;
        if (frac > 1.0f) frac = 1.0f;
        if (frac < 0.0f) frac = 0.0f;
        ImU32 bc = (frac > 0.6f) ? IM_COL32(46,204,113,255)
                 : (frac > 0.3f) ? IM_COL32(241,196,15,255)
                                 : IM_COL32(231,76,60,255);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImColor(bc).Value);
        char ov[32]; std::snprintf(ov, sizeof(ov), "%d ticks", o.ticksLeft);
        ImGui::ProgressBar(frac, ImVec2(-1, 14), ov);
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::Spacing();
    }

    ImGui::End();
}
