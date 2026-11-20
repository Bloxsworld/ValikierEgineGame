#pragma once
#include <string>
#include <vector>

struct ManagedScriptItem {
    std::string name;
    std::string filePath;
    std::string category = "Gameplay3D";
    bool enabled = true;
    int ticksRun = 0;
};

class ScriptManager {
public:
    bool RegisterScript(const std::string& name, const std::string& filePath, const std::string& category = "Gameplay3D");
    bool SetScriptEnabled(const std::string& name, bool enabled);
    void TickAllEnabled(float dt);
    void PrintGuiScriptList() const;
    size_t Count() const { return m_Scripts.size(); }
    size_t EnabledCount() const;
    const std::vector<ManagedScriptItem>& GetScripts() const { return m_Scripts; }

private:
    std::vector<ManagedScriptItem> m_Scripts;
};