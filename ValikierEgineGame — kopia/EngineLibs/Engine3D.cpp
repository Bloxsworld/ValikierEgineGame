#include "Engine3D.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdio>

namespace {
    std::string GetNowStr() {
        std::time_t now = std::time(nullptr);
        char buf[32]{};
        std::tm tmLocal{};
        localtime_s(&tmLocal, &now);
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmLocal);
        return std::string(buf);
    }

    void CopyAllFilesInDir(const std::string& src, const std::string& dst) {
        CreateDirectoryA(dst.c_str(), nullptr);
        std::string pattern = src + "\\*.*";
        WIN32_FIND_DATAA ffd{};
        HANDLE hFind = FindFirstFileA(pattern.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE) return;

        do {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string from = src + "\\" + ffd.cFileName;
                std::string to = dst + "\\" + ffd.cFileName;
                CopyFileA(from.c_str(), to.c_str(), FALSE);
            }
        } while (FindNextFileA(hFind, &ffd));

        FindClose(hFind);
    }
}

ValkyrieEngineApp::~ValkyrieEngineApp() {
    UnloadEngineDlls();
}

std::string ValkyrieEngineApp::NormalizeExeName(const std::string& input) {
    if (input.empty()) return "Valkyrie3DGame.exe";
    if (input.size() >= 4) {
        std::string tail = input.substr(input.size() - 4);
        if (tail == ".exe" || tail == ".EXE") {
            return input;
        }
    }
    return input + ".exe";
}

bool ValkyrieEngineApp::RunProcessWait(const std::string& cmdLine) {
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::string mutableCmd = cmdLine;
    BOOL ok = CreateProcessA(
        nullptr,
        mutableCmd.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!ok) return false;

    WaitForSingleObject(pi.hProcess, 20000);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return exitCode == 0;
}

void ValkyrieEngineApp::EnsureDefaultEngineDllSource(const std::string& srcPath, const std::string& dllName) const {
    std::ifstream check(srcPath);
    if (check.is_open()) return;

    std::ofstream out(srcPath);
    if (!out.is_open()) return;

    out << "#include <windows.h>\n"
        << "#include <cstdio>\n\n"
        << "namespace {\n"
        << "    int g_Step = 0;\n"
        << "    float g_Elapsed = 0.0f;\n"
        << "}\n\n"
        << "extern \"C\" __declspec(dllexport) const char* EngineDll_Info() {\n"
        << "    return \"" << dllName << " | ValkyrieEngine Dynamic Library\";\n"
        << "}\n\n"
        << "extern \"C\" __declspec(dllexport) bool EngineDll_Init() {\n"
        << "    g_Step = 0;\n"
        << "    g_Elapsed = 0.0f;\n"
        << "    return true;\n"
        << "}\n\n"
        << "extern \"C\" __declspec(dllexport) int EngineDll_Step(float dt, char* outBuf, int outSize) {\n"
        << "    g_Step++;\n"
        << "    g_Elapsed += dt;\n"
        << "    if (outBuf && outSize > 0) {\n"
        << "        snprintf(outBuf, static_cast<size_t>(outSize), \"[" << dllName
        << "] step=%d | t=%.2fs\", g_Step, g_Elapsed);\n"
        << "    }\n"
        << "    return g_Step;\n"
        << "}\n\n"
        << "BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID res) {\n"
        << "    (void)hMod; (void)reason; (void)res;\n"
        << "    return TRUE;\n"
        << "}\n";
}

bool ValkyrieEngineApp::CompileAndAttachEngineDll(const std::string& dllBaseName) {
    CreateDirectoryA(m_EngineLibsDir.c_str(), nullptr);
    std::string srcPath = m_EngineLibsDir + "\\" + dllBaseName + "_src.cpp";
    std::string outDllPath = m_EngineLibsDir + "\\" + dllBaseName + ".dll";

    EnsureDefaultEngineDllSource(srcPath, dllBaseName + ".dll");
    std::cout << "[EngineBuild] Kompiluje biblioteke DLL: " << outDllPath << "\n";

    std::string cmdGpp = "cmd.exe /c g++ -shared -std=c++17 \"" + srcPath + "\" -o \"" + outDllPath + "\"";
    std::string cmdMsvc = "cmd.exe /c cl /nologo /LD /EHsc \"" + srcPath + "\" /Fe:\"" + outDllPath + "\"";

    bool built = RunProcessWait(cmdGpp) || RunProcessWait(cmdMsvc);
    if (!built) {
        std::cout << "[EngineBuild] Brak g++/cl.exe w PATH. Zapisano zrodlo: " << srcPath << "\n";
        return false;
    }

    std::cout << "[EngineBuild] Sukces! Zbudowano: " << outDllPath << "\n";
    return AttachEngineDll(outDllPath);
}

bool ValkyrieEngineApp::AttachEngineDll(const std::string& dllPath) {
    HMODULE hMod = LoadLibraryA(dllPath.c_str());
    if (!hMod) {
        std::cout << "[EngineDLL] Nie udalo sie zaladowac DLL: " << dllPath
            << " (kod: " << GetLastError() << ")\n";
        return false;
    }

    LoadedEngineDll item;
    item.path = dllPath;
    item.handle = hMod;
    item.info = reinterpret_cast<FnEngineDllInfo>(GetProcAddress(hMod, "EngineDll_Info"));
    item.init = reinterpret_cast<FnEngineDllInit>(GetProcAddress(hMod, "EngineDll_Init"));
    item.step = reinterpret_cast<FnEngineDllStep>(GetProcAddress(hMod, "EngineDll_Step"));

    if (item.init) {
        item.init();
    }

    std::cout << "[EngineDLL] Podpieto biblioteke: "
        << (item.info ? item.info() : dllPath) << "\n";

    m_EngineDlls.push_back(item);
    return true;
}

void ValkyrieEngineApp::TickEngineDlls(float dt) {
    if (m_EngineDlls.empty()) {
        std::cout << "[EngineDLL] Brak podpietych bibliotek DLL silnika.\n";
        return;
    }

    for (auto& dll : m_EngineDlls) {
        if (dll.step) {
            char buf[256]{};
            dll.step(dt, buf, sizeof(buf));
            if (buf[0] != '\0') {
                std::cout << buf << "\n";
            }
        }
    }
}

void ValkyrieEngineApp::UnloadEngineDlls() {
    for (auto& dll : m_EngineDlls) {
        if (dll.handle) {
            FreeLibrary(dll.handle);
            dll.handle = nullptr;
        }
    }
    m_EngineDlls.clear();
}

bool ValkyrieEngineApp::Init(const std::string& rootDir) {
    m_RootDir = rootDir;
    m_CoreConfigPath = m_RootDir + "\\Core\\config.ini";
    m_EngineLibsDir = m_RootDir + "\\EngineLibs";

    CreateDirectoryA((m_RootDir + "\\Core").c_str(), nullptr);
    CreateDirectoryA((m_RootDir + "\\FakeInject").c_str(), nullptr);
    CreateDirectoryA((m_RootDir + "\\Scenes").c_str(), nullptr);
    CreateDirectoryA((m_RootDir + "\\Scripts").c_str(), nullptr);
    CreateDirectoryA(m_EngineLibsDir.c_str(), nullptr);

    LoadConfig();
    LoadSceneFile("Scenes\\IntroMadeBy.vescene");
    LoadSceneFile("Scenes\\Main3D.vescene");
    return true;
}

void ValkyrieEngineApp::LoadConfig() {
    char projBuf[128]{};
    GetPrivateProfileStringA("Project", "Name", "Valkyrie3DGame", projBuf, sizeof(projBuf), m_CoreConfigPath.c_str());
    m_Settings.projectName = projBuf;

    std::string defaultExe = NormalizeExeName(m_Settings.projectName);
    char procBuf[MAX_PATH]{};
    GetPrivateProfileStringA("Target", "TargetProcessName", defaultExe.c_str(),
        procBuf, sizeof(procBuf), m_CoreConfigPath.c_str());
    m_Settings.compiledGameExe = NormalizeExeName(procBuf);

    m_Settings.antiCheatEnabled = GetPrivateProfileIntA("AntiCheat", "Enabled", 1, m_CoreConfigPath.c_str()) != 0;
    m_Settings.banOnInject = GetPrivateProfileIntA("AntiCheat", "BanOnInject", 1, m_CoreConfigPath.c_str()) != 0;
    m_Settings.scanIntervalMs = GetPrivateProfileIntA("AntiCheat", "ScanIntervalMs", 500, m_CoreConfigPath.c_str());
}

void ValkyrieEngineApp::SaveConfig() {
    WritePrivateProfileStringA("Project", "Name", m_Settings.projectName.c_str(), m_CoreConfigPath.c_str());
    WritePrivateProfileStringA("Target", "TargetProcessName", m_Settings.compiledGameExe.c_str(), m_CoreConfigPath.c_str());
    WritePrivateProfileStringA("AntiCheat", "Enabled", m_Settings.antiCheatEnabled ? "1" : "0", m_CoreConfigPath.c_str());
    WritePrivateProfileStringA("AntiCheat", "BanOnInject", m_Settings.banOnInject ? "1" : "0", m_CoreConfigPath.c_str());

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", m_Settings.scanIntervalMs);
    WritePrivateProfileStringA("AntiCheat", "ScanIntervalMs", buf, m_CoreConfigPath.c_str());
}

void ValkyrieEngineApp::SetCompiledGameName(const std::string& gameNameOrExe) {
    m_Settings.compiledGameExe = NormalizeExeName(gameNameOrExe);
    if (gameNameOrExe.size() > 4 && gameNameOrExe.substr(gameNameOrExe.size() - 4) == ".exe") {
        m_Settings.projectName = gameNameOrExe.substr(0, gameNameOrExe.size() - 4);
    }
    else if (!gameNameOrExe.empty()) {
        m_Settings.projectName = gameNameOrExe;
    }
    SaveConfig();
    std::cout << "[Build] Oczekiwany proces skompilowanej gry: " << m_Settings.compiledGameExe << "\n";
}

bool ValkyrieEngineApp::LoadSceneFile(const std::string& sceneRelativePath) {
    std::string fullPath = m_RootDir + "\\" + sceneRelativePath;
    std::ifstream in(fullPath);
    if (!in.is_open()) {
        std::cout << "[Scene] Nie znaleziono pliku sceny: " << fullPath << "\n";
        return false;
    }

    std::string line;
    int count = 0;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "SceneBanner") {
            SceneEntity3D e;
            e.kind = "Banner";
            iss >> e.name >> e.pos.x >> e.pos.y >> e.pos.z;
            m_LoadedEntities.push_back(e);
            count++;
        }
        else if (tag == "Object3D") {
            SceneEntity3D e;
            e.kind = "Object3D";
            float sx = 1.0f, sy = 1.0f, sz = 1.0f;
            if (iss >> e.name >> e.detail >> e.pos.x >> e.pos.y >> e.pos.z >> sx >> sy >> sz) {
                m_LoadedEntities.push_back(e);
                count++;
            }
        }
    }

    std::cout << "[Scene] Wczytano " << count << " wpisow z " << sceneRelativePath << "\n";
    return true;
}

void ValkyrieEngineApp::SetBanOnInject(bool enabled) {
    m_Settings.banOnInject = enabled;
    SaveConfig();
    std::cout << "[Config] Zaktualizowano BanOnInject=" << (enabled ? "1" : "0") << " w Core\\config.ini\n";
}

bool ValkyrieEngineApp::RunFakeInject(const std::string& dllFileName) {
    std::string dllPath = m_RootDir + "\\FakeInject\\" + dllFileName;
    HMODULE hMod = LoadLibraryA(dllPath.c_str());
    if (!hMod) {
        std::cout << "[FakeInject] Brak lub blad ladowania DLL: " << dllPath
            << " (kod: " << GetLastError() << ")\n";
        return false;
    }

    std::cout << "[FakeInject] Zaladowano testowy modul: " << dllFileName
        << " (cel gry: " << m_Settings.compiledGameExe << ")\n";

    if (m_Settings.antiCheatEnabled && m_Settings.banOnInject) {
        std::ofstream bans(m_RootDir + "\\bans.txt", std::ios::app);
        if (bans.is_open()) {
            bans << "[" << GetNowStr() << "] [BAN] GameExe=" << m_Settings.compiledGameExe
                << " TestInject=" << dllFileName << "\n";
        }
        std::cout << "[AC] Tryb BAN aktywny -> dopisano bana dla " << m_Settings.compiledGameExe << " do bans.txt!\n";
    }
    else if (m_Settings.antiCheatEnabled) {
        std::cout << "[AC] Wykryto testowy inject (" << dllFileName << ") dla " << m_Settings.compiledGameExe
            << ", ale BanOnInject=0.\n";
    }
    else {
        std::cout << "[AC] Anty-cheat wylaczony w config.ini.\n";
    }

    FreeLibrary(hMod);
    return true;
}

bool ValkyrieEngineApp::ExportGameBundle(const std::string& outDir) {
    SaveConfig();
    CreateDirectoryA(outDir.c_str(), nullptr);
    CopyAllFilesInDir(m_RootDir + "\\Core", outDir + "\\Core");
    CopyAllFilesInDir(m_RootDir + "\\Scenes", outDir + "\\Scenes");
    CopyAllFilesInDir(m_RootDir + "\\Scripts", outDir + "\\Scripts");
    CopyAllFilesInDir(m_EngineLibsDir, outDir + "\\EngineLibs");
    std::cout << "[Export] Skopiowano Core, Scenes, Scripts i EngineLibs dla gry ["
        << m_Settings.compiledGameExe << "] do: " << outDir << "\n";
    return true;
}

void ValkyrieEngineApp::ShowSummary() const {
    std::cout << "\n=== ValkyrieEngine Summary ===\n"
        << "Projekt: " << m_Settings.projectName << "\n"
        << "Skompilowana gra (cel): " << m_Settings.compiledGameExe << "\n"
        << "Wczytane elementy scen: " << m_LoadedEntities.size() << "\n"
        << "Podpiete DLL silnika: " << m_EngineDlls.size() << "\n"
        << "Anty-Cheat: " << (m_Settings.antiCheatEnabled ? "ON" : "OFF") << "\n"
        << "Ban za inject: " << (m_Settings.banOnInject ? "ON" : "OFF") << "\n\n";
}