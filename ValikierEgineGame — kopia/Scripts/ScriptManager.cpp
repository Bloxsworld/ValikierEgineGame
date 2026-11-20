#include "ScriptManager.h"
#include "Logger3D.h"
#include <iostream>
#include <algorithm>

bool ScriptManager::RegisterScript(const std::string& name, const std::string& filePath, const std::string& category) {
    if (name.empty()) {
        Logger3D::Instance().Warn("ScriptManager", "Cannot register script with empty name.");
        return false;
    }

    for (auto& item : m_Scripts) {
        if (item.name == name) {
            item.filePath = filePath;
            item.category = category;
            Logger3D::Instance().Info("ScriptManager", "Updated existing script entry: " + name + " (" + filePath + ")");
            return true;
        }
    }

    ManagedScriptItem entry;
    entry.name = name;
    entry.filePath = filePath;
    entry.category = category;
    entry.enabled = true;
    entry.ticksRun = 0;
    m_Scripts.push_back(entry);

    Logger3D::Instance().Info("ScriptManager", "Registered script [" + name + "] -> " + filePath);
    return true;
}

bool ScriptManager::SetScriptEnabled(const std::string& name, bool enabled) {
    for (auto& item : m_Scripts) {
        if (item.name == name) {
            item.enabled = enabled;
            Logger3D::Instance().Gui("ScriptManager",
                "Script [" + name + "] state set to " + (enabled ? "ENABLED" : "DISABLED"));
            return true;
        }
    }
    Logger3D::Instance().Warn("ScriptManager", "Script not found: " + name);
    return false;
}

void ScriptManager::TickAllEnabled(float dt) {
    (void)dt;
    for (auto& item : m_Scripts) {
        if (!item.enabled) continue;
        item.ticksRun++;
    }
}

void ScriptManager::PrintGuiScriptList() const {
    Logger3D::Instance().Gui("ScriptManager",
        "Rendering ScriptManager GUI list (" + std::to_string(EnabledCount()) + "/" +
        std::to_string(m_Scripts.size()) + " active)");

    std::cout << "+------------------- [GUI:ScriptManager] ----------------------+\n";
    if (m_Scripts.empty()) {
        std::cout << "| Brak zarejestrowanych skryptow.                                |\n";
    }
    else {
        for (const auto& s : m_Scripts) {
            std::cout << "| [" << (s.enabled ? "ON " : "OFF") << "] "
                << s.name << " (" << s.category << ") | Ticks: " << s.ticksRun
                << " | " << s.filePath << "\n";
        }
    }
    std::cout << "+--------------------------------------------------------------+\n";
}

size_t ScriptManager::EnabledCount() const {
    size_t c = 0;
    for (const auto& s : m_Scripts) {
        if (s.enabled) c++;
    }
    return c;
}