#pragma once
#include "Logger3D.h"
#include "AnimMove3D.h"
#include "ScriptManager.h"
#include <iostream>
#include <string>
#include <vector>

struct GuiWidget3D {
    std::string id;
    std::string title;
    int x = 0;
    int y = 0;
    int width = 200;
    int height = 100;
    bool visible = true;
};

class GuiSystem3D {
public:
    float hp = 100.0f;
    float stamina = 85.0f;
    int fps = 120;
    std::string activeTool = "MoveTool3D";
    bool showConsole = true;
    bool showScriptManager = true;

    void InitDefaultHud() {
        m_Widgets.clear();
        m_Widgets.push_back({ "hud_stats", "Player HUD", 10, 10, 260, 80, true });
        m_Widgets.push_back({ "anim_inspector", "AnimMove3D Inspector", 280, 10, 300, 120, true });
        m_Widgets.push_back({ "script_manager", "Script Manager", 10, 135, 570, 140, true });
        m_Widgets.push_back({ "log_console", "Runtime Log Console", 10, 280, 570, 180, true });
        Logger3D::Instance().Gui("GuiSystem3D", "Default 3D HUD, ScriptManager & Console widgets initialized (" +
            std::to_string(m_Widgets.size()) + " panels)");
    }

    void ToggleConsole() {
        showConsole = !showConsole;
        for (auto& w : m_Widgets) {
            if (w.id == "log_console") w.visible = showConsole;
        }
        Logger3D::Instance().Gui("GuiSystem3D", std::string("Console visibility -> ") + (showConsole ? "ON" : "OFF"));
    }

    void ToggleScriptManager() {
        showScriptManager = !showScriptManager;
        for (auto& w : m_Widgets) {
            if (w.id == "script_manager") w.visible = showScriptManager;
        }
        Logger3D::Instance().Gui("GuiSystem3D", std::string("ScriptManager GUI -> ") + (showScriptManager ? "ON" : "OFF"));
    }

    void RenderFrameGui(const AnimMove3D& anim, const ScriptManager* scriptMgr, float posX, float posY, float posZ) const {
        const LimbPose3D& pose = anim.GetPose();
        std::cout << "\n+--------------------------------------------------------------+\n";
        std::cout << "| [GUI:Player HUD] HP: " << hp << " | Stamina: " << stamina << " | FPS: " << fps << "\n";
        std::cout << "| [GUI:World3D]    Pos: (" << posX << ", " << posY << ", " << posZ << ") | Tool: " << activeTool << "\n";
        std::cout << "| [GUI:AnimMove3D] State: " << anim.StateName(anim.state)
            << " | Steps: " << anim.footstepCount
            << " | LegL: " << pose.leftLegPitch
            << " | BobY: " << pose.torsoBobY << "\n";

        if (showScriptManager && scriptMgr) {
            std::cout << "| [GUI:ScriptMgr]  Scripts: " << scriptMgr->EnabledCount()
                << "/" << scriptMgr->Count() << " active\n";
            for (const auto& s : scriptMgr->GetScripts()) {
                std::cout << "|   - [" << (s.enabled ? "ON " : "OFF") << "] "
                    << s.name << " (ticks=" << s.ticksRun << ")\n";
            }
        }

        if (showConsole) {
            std::cout << "+------------------- [GUI:Log Console] ------------------------+\n";
            auto recent = Logger3D::Instance().GetRecentFormatted(4);
            for (const auto& line : recent) {
                std::cout << "| " << line << "\n";
            }
        }
        std::cout << "+--------------------------------------------------------------+\n";
    }

private:
    std::vector<GuiWidget3D> m_Widgets;
};