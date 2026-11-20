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

    // Kompilacja pelnej gry 3D do folderu Games\<NazwaGry>\<NazwaGry>.exe
    bool CompileGame3D(const std::string& optionalGameName = "");

    // Tworzenie kodu C++ i kompilacja w EngineLibs oraz Scripts
    bool CreateEngineDllSource(const std::string& dllBaseName);
    bool CompileAndAttachEngineDll(const std::string& dllBaseName = "ValkyrieEngine3D");
    bool AttachEngineDll(const std::string& dllPath);
    void TickEngineDlls(float dt = 0.1f);
    void UnloadEngineDlls();

    bool CreateScriptSource(const std::string& scriptBaseName);
    bool CompileScript(const std::string& scriptBaseName);

    void ListWorkspaceFolders() const;
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
    std::string m_CoreDir;
    std::string m_CoreConfigPath;
    std::string m_EngineLibsDir;
    std::string m_FakeInjectDir;
    std::string m_ScenesDir;
    std::string m_ScriptsDir;
    std::string m_GamesDir;

    EngineSettings m_Settings;
    std::vector<SceneEntity3D> m_LoadedEntities;
    std::vector<LoadedEngineDll> m_EngineDlls;

    static std::string NormalizeExeName(const std::string& input);
    static std::string StripExeExtension(const std::string& input);
    static std::string FindVsDevCmd();
    bool RunCompilerCommand(const std::string& gppCmd, const std::string& msvcCmd) const;
    void WriteGame3DSource(const std::string& srcPath, const std::string& gameName) const;
    void LoadConfig();
    void SaveConfig();
};