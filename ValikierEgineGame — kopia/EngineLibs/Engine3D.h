#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct SceneEntity3D {
    std::string kind;
    std::string name;
    std::string detail;
    Vec3 pos{};
};

struct EngineSettings {
    std::string projectName = "Valkyrie3DGame";
    std::string compiledGameExe = "Valkyrie3DGame.exe";
    bool antiCheatEnabled = true;
    bool banOnInject = true;
    int scanIntervalMs = 500;
};

class ValkyrieEngineApp {
public:
    ~ValkyrieEngineApp();

    bool Init(const std::string& rootDir);
    bool LoadSceneFile(const std::string& sceneRelativePath);
    void SetCompiledGameName(const std::string& gameNameOrExe);
    void SetBanOnInject(bool enabled);
    bool RunFakeInject(const std::string& dllFileName);
    bool ExportGameBundle(const std::string& outDir);

    // Kompilacja i dolaczanie bibliotek .dll silnika
    bool CompileAndAttachEngineDll(const std::string& dllBaseName = "ValkyrieEngine3D");
    bool AttachEngineDll(const std::string& dllPath);
    void TickEngineDlls(float dt = 0.1f);
    void UnloadEngineDlls();

    void ShowSummary() const;

private:
    typedef const char* (*FnEngineDllInfo)();
    typedef bool        (*FnEngineDllInit)();
    typedef int         (*FnEngineDllStep)(float dt, char* outBuf, int outSize);

    struct LoadedEngineDll {
        std::string path;
        HMODULE handle = nullptr;
        FnEngineDllInfo info = nullptr;
        FnEngineDllInit init = nullptr;
        FnEngineDllStep step = nullptr;
    };

    std::string m_RootDir;
    std::string m_CoreConfigPath;
    std::string m_EngineLibsDir;
    EngineSettings m_Settings;
    std::vector<SceneEntity3D> m_LoadedEntities;
    std::vector<LoadedEngineDll> m_EngineDlls;

    static std::string NormalizeExeName(const std::string& input);
    static bool RunProcessWait(const std::string& cmdLine);
    void EnsureDefaultEngineDllSource(const std::string& srcPath, const std::string& dllName) const;
    void LoadConfig();
    void SaveConfig();
};