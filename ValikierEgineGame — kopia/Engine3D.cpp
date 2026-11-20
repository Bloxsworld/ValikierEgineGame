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

    void PrintDirFiles(const std::string& label, const std::string& dir) {
        std::cout << "  [" << label << "] (" << dir << "):\n";
        std::string pattern = dir + "\\*.*";
        WIN32_FIND_DATAA ffd{};
        HANDLE hFind = FindFirstFileA(pattern.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE) {
            std::cout << "    (brak dostepu lub pusty)\n";
            return;
        }
        int count = 0;
        do {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::cout << "    - " << ffd.cFileName << "\n";
                count++;
            }
        } while (FindNextFileA(hFind, &ffd));
        FindClose(hFind);
        if (count == 0) {
            std::cout << "    (brak plikow)\n";
        }
    }

    bool ExecBatOrCmd(const std::string& fullCmd) {
        STARTUPINFOA si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        std::string cmd = "cmd.exe /c " + fullCmd;
        BOOL ok = CreateProcessA(
            nullptr,
            cmd.data(),
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

        WaitForSingleObject(pi.hProcess, 30000);
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return exitCode == 0;
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

std::string ValkyrieEngineApp::StripExeExtension(const std::string& input) {
    if (input.size() >= 4) {
        std::string tail = input.substr(input.size() - 4);
        if (tail == ".exe" || tail == ".EXE") {
            return input.substr(0, input.size() - 4);
        }
    }
    return input;
}

std::string ValkyrieEngineApp::FindVsDevCmd() {
    const char* candidates[] = {
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\Tools\\VsDevCmd.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\Common7\\Tools\\VsDevCmd.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\Common7\\Tools\\VsDevCmd.bat",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\Tools\\VsDevCmd.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\Tools\\VsDevCmd.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\18\\BuildTools\\Common7\\Tools\\VsDevCmd.bat"
    };

    for (const char* p : candidates) {
        DWORD attr = GetFileAttributesA(p);
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            return std::string(p);
        }
    }
    return "";
}

bool ValkyrieEngineApp::RunCompilerCommand(const std::string& gppCmd, const std::string& msvcCmd) const {
    if (ExecBatOrCmd("where g++ >nul 2>nul && " + gppCmd)) {
        return true;
    }

    if (ExecBatOrCmd("where cl >nul 2>nul && " + msvcCmd)) {
        return true;
    }

    std::string vsDev = FindVsDevCmd();
    if (!vsDev.empty()) {
        std::string wrapped = "(\"" + vsDev + "\" -arch=x64 -host_arch=x64 >nul && " + msvcCmd + ")";
        if (ExecBatOrCmd(wrapped)) {
            return true;
        }
    }

    return false;
}

void ValkyrieEngineApp::WriteGame3DSource(const std::string& srcPath, const std::string& gameName) const {
    std::ofstream out(srcPath);
    if (!out.is_open()) return;

    out << "#include <windows.h>\n"
        << "#include <iostream>\n"
        << "#include <fstream>\n"
        << "#include <string>\n\n"
        << "static void PrintSceneSummary(const std::string& sceneFile) {\n"
        << "    std::ifstream in(sceneFile);\n"
        << "    if (!in.is_open()) {\n"
        << "        std::cout << \"  [Scene] Brak pliku: \" << sceneFile << \"\\n\";\n"
        << "        return;\n"
        << "    }\n"
        << "    std::cout << \"  [Scene] Wczytano: \" << sceneFile << \"\\n\";\n"
        << "    std::string line;\n"
        << "    while (std::getline(in, line)) {\n"
        << "        if (line.empty() || line[0] == '#') continue;\n"
        << "        std::cout << \"    3D -> \" << line << \"\\n\";\n"
        << "    }\n"
        << "}\n\n"
        << "int main() {\n"
        << "    SetConsoleTitleA(\"" << gameName << " - ValkyrieEngine 3D Build\");\n"
        << "    std::cout << \"========================================\\n\";\n"
        << "    std::cout << \"  GRA 3D: " << gameName << "\\n\";\n"
        << "    std::cout << \"  Silnik: ValkyrieEngine 3D Runtime\\n\";\n"
        << "    std::cout << \"========================================\\n\";\n\n"
        << "    char exeBuf[MAX_PATH]{};\n"
        << "    GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);\n"
        << "    std::string exePath(exeBuf);\n"
        << "    size_t pos = exePath.find_last_of(\"\\\\/\");\n"
        << "    std::string gameDir = (pos != std::string::npos) ? exePath.substr(0, pos) : \".\";\n\n"
        << "    PrintSceneSummary(gameDir + \"\\\\Scenes\\\\IntroMadeBy.vescene\");\n"
        << "    PrintSceneSummary(gameDir + \"\\\\Scenes\\\\Main3D.vescene\");\n\n"
        << "    HMODULE hEngineDll = LoadLibraryA((gameDir + \"\\\\EngineLibs\\\\ValkyrieEngine3D.dll\").c_str());\n"
        << "    if (hEngineDll) {\n"
        << "        std::cout << \"  [EngineDLL] Zaladowano ValkyrieEngine3D.dll\\n\";\n"
        << "        FreeLibrary(hEngineDll);\n"
        << "    }\n\n"
        << "    std::cout << \"\\n  Swiat 3D dla gry [" << gameName << "] uruchomiony poprawnie!\\n\";\n"
        << "    std::cout << \"  Nacisnij ENTER aby zamknac...\\n\";\n"
        << "    std::cin.get();\n"
        << "    return 0;\n"
        << "}\n";
}

bool ValkyrieEngineApp::CompileGame3D(const std::string& optionalGameName) {
    if (!optionalGameName.empty()) {
        SetCompiledGameName(optionalGameName);
    }

    std::string baseName = StripExeExtension(m_Settings.projectName);
    if (baseName.empty()) baseName = "Valkyrie3DGame";
    std::string exeName = baseName + ".exe";

    CreateDirectoryA(m_GamesDir.c_str(), nullptr);
    std::string gameFolder = m_GamesDir + "\\" + baseName;
    CreateDirectoryA(gameFolder.c_str(), nullptr);

    std::string srcPath = gameFolder + "\\" + baseName + "_3D.cpp";
    std::string outExePath = gameFolder + "\\" + exeName;

    std::ifstream existingSrc(srcPath);
    if (!existingSrc.is_open()) {
        WriteGame3DSource(srcPath, baseName);
    }

    std::cout << "[Compile3D] Kompiluje gre 3D [" << baseName << "] do: " << outExePath << "\n";

    std::string gppCmd = "g++ -std=c++17 -O2 \"" + srcPath + "\" -o \"" + outExePath + "\"";
    std::string msvcCmd = "cl /nologo /std:c++17 /EHsc /O2 \"" + srcPath +
        "\" /Fe:\"" + outExePath + "\" /Fo\"" + gameFolder + "\\\"";

    if (!RunCompilerCommand(gppCmd, msvcCmd)) {
        std::cout << "[BLAD] Nie udalo sie skompilowac gry 3D: " << outExePath << "\n";
        return false;
    }

    ExportGameBundle(gameFolder);
    std::cout << "[Compile3D] Gotowe! Gra 3D znajduje sie w: " << gameFolder << "\n";
    return true;
}

bool ValkyrieEngineApp::CreateEngineDllSource(const std::string& dllBaseName) {
    CreateDirectoryA(m_EngineLibsDir.c_str(), nullptr);
    std::string srcPath = m_EngineLibsDir + "\\" + dllBaseName + ".cpp";

    std::ofstream out(srcPath);
    if (!out.is_open()) {
        std::cout << "[BLAD] Nie mozna utworzyc pliku: " << srcPath << "\n";
        return false;
    }

    out << "#include <windows.h>\n"
        << "#include <cstdio>\n\n"
        << "namespace {\n"
        << "    int g_Tick = 0;\n"
        << "    float g_Time = 0.0f;\n"
        << "}\n\n"
        << "extern \"C\" __declspec(dllexport) const char* EngineDll_Info() {\n"
        << "    return \"" << dllBaseName << ".dll | ValkyrieEngine Module\";\n"
        << "}\n\n"
        << "extern \"C\" __declspec(dllexport) bool EngineDll_Init() {\n"
        << "    g_Tick = 0;\n"
        << "    g_Time = 0.0f;\n"
        << "    return true;\n"
        << "}\n\n"
        << "extern \"C\" __declspec(dllexport) int EngineDll_Step(float dt, char* outBuf, int outSize) {\n"
        << "    g_Tick++;\n"
        << "    g_Time += dt;\n"
        << "    if (outBuf && outSize > 0) {\n"
        << "        snprintf(outBuf, static_cast<size_t>(outSize), \"[" << dllBaseName
        << ".dll] tick=%d | t=%.2fs\", g_Tick, g_Time);\n"
        << "    }\n"
        << "    return g_Tick;\n"
        << "}\n\n"
        << "BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID res) {\n"
        << "    (void)hMod; (void)reason; (void)res;\n"
        << "    return TRUE;\n"
        << "}\n";

    std::cout << "[CodeGen] Utworzono kod C++ DLL w EngineLibs: " << srcPath << "\n";
    return true;
}

bool ValkyrieEngineApp::CompileAndAttachEngineDll(const std::string& dllBaseName) {
    CreateDirectoryA(m_EngineLibsDir.c_str(), nullptr);
    std::string srcPath = m_EngineLibsDir + "\\" + dllBaseName + ".cpp";
    std::string outDllPath = m_EngineLibsDir + "\\" + dllBaseName + ".dll";

    std::ifstream checkSrc(srcPath);
    if (!checkSrc.is_open()) {
        std::cout << "[EngineBuild] Brak " << srcPath << " -> tworze nowy kod C++...\n";
        CreateEngineDllSource(dllBaseName);
    }

    std::cout << "[EngineBuild] Kompiluje " << srcPath << " -> " << outDllPath << "\n";

    std::string gppCmd = "g++ -shared -std=c++17 \"" + srcPath + "\" -o \"" + outDllPath + "\"";
    std::string msvcCmd = "cl /nologo /std:c++17 /LD /EHsc \"" + srcPath +
        "\" /Fe:\"" + outDllPath + "\" /Fo\"" + m_EngineLibsDir + "\\\"";

    if (!RunCompilerCommand(gppCmd, msvcCmd)) {
        std::cout << "[BLAD] Kompilacja DLL nie powiodla sie. Sprawdz kod w " << srcPath
            << " albo kompilator (MSVC / g++).\n";
        return false;
    }

    std::cout << "[EngineBuild] Sukces! Zbudowano: " << outDllPath << "\n";
    return AttachEngineDll(outDllPath);
}

bool ValkyrieEngineApp::AttachEngineDll(const std::string& dllPath) {
    std::string resolved = dllPath;
    DWORD attr = GetFileAttributesA(resolved.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        std::string inEngineLibs = m_EngineLibsDir + "\\" + dllPath;
        if (GetFileAttributesA(inEngineLibs.c_str()) != INVALID_FILE_ATTRIBUTES) {
            resolved = inEngineLibs;
        }
    }

    HMODULE hMod = LoadLibraryA(resolved.c_str());
    if (!hMod) {
        std::cout << "[EngineDLL] Nie udalo sie zaladowac DLL: " << resolved
            << " (kod: " << GetLastError() << ")\n";
        return false;
    }

    LoadedEngineDll item;
    item.path = resolved;
    item.handle = hMod;
    item.info = reinterpret_cast<FnEngineDllInfo>(GetProcAddress(hMod, "EngineDll_Info"));
    item.init = reinterpret_cast<FnEngineDllInit>(GetProcAddress(hMod, "EngineDll_Init"));
    item.step = reinterpret_cast<FnEngineDllStep>(GetProcAddress(hMod, "EngineDll_Step"));

    if (item.init) {
        item.init();
    }

    std::cout << "[EngineDLL] Podpieto: " << (item.info ? item.info() : resolved) << "\n";
    m_EngineDlls.push_back(item);
    return true;
}

void ValkyrieEngineApp::TickEngineDlls(float dt) {
    if (m_EngineDlls.empty()) {
        std::cout << "[EngineDLL] Brak podpietych DLL silnika. Uzyj: builddll <Nazwa>\n";
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

bool ValkyrieEngineApp::CreateScriptSource(const std::string& scriptBaseName) {
    CreateDirectoryA(m_ScriptsDir.c_str(), nullptr);
    std::string srcPath = m_ScriptsDir + "\\" + scriptBaseName + ".cpp";

    std::ofstream out(srcPath);
    if (!out.is_open()) {
        std::cout << "[BLAD] Nie mozna utworzyc skryptu: " << srcPath << "\n";
        return false;
    }

    out << "#include <iostream>\n\n"
        << "int main() {\n"
        << "    std::cout << \"[Script:" << scriptBaseName << "] Uruchomiono skrypt gry z Scripts!\\n\";\n"
        << "    return 0;\n"
        << "}\n";

    std::cout << "[CodeGen] Utworzono skrypt C++ w Scripts: " << srcPath << "\n";
    return true;
}

bool ValkyrieEngineApp::CompileScript(const std::string& scriptBaseName) {
    CreateDirectoryA(m_ScriptsDir.c_str(), nullptr);
    std::string srcPath = m_ScriptsDir + "\\" + scriptBaseName + ".cpp";
    std::string exePath = m_ScriptsDir + "\\" + scriptBaseName + ".exe";

    std::ifstream checkSrc(srcPath);
    if (!checkSrc.is_open()) {
        std::cout << "[ScriptBuild] Brak " << srcPath << " -> tworze nowy skrypt C++...\n";
        CreateScriptSource(scriptBaseName);
    }

    std::cout << "[ScriptBuild] Kompiluje " << srcPath << " -> " << exePath << "\n";

    std::string gppCmd = "g++ -std=c++17 \"" + srcPath + "\" -o \"" + exePath + "\"";
    std::string msvcCmd = "cl /nologo /std:c++17 /EHsc \"" + srcPath +
        "\" /Fe:\"" + exePath + "\" /Fo\"" + m_ScriptsDir + "\\\"";

    if (!RunCompilerCommand(gppCmd, msvcCmd)) {
        std::cout << "[BLAD] Kompilacja skryptu nie powiodla sie: " << srcPath << "\n";
        return false;
    }

    std::cout << "[ScriptBuild] Sukces! Skompilowano: " << exePath << "\n";
    return true;
}

void ValkyrieEngineApp::ListWorkspaceFolders() const {
    std::cout << "\n=== Pliki we wszystkich folderach projektu ===\n";
    PrintDirFiles("Core", m_CoreDir);
    PrintDirFiles("EngineLibs", m_EngineLibsDir);
    PrintDirFiles("FakeInject", m_FakeInjectDir);
    PrintDirFiles("Scenes", m_ScenesDir);
    PrintDirFiles("Scripts", m_ScriptsDir);
    PrintDirFiles("Games", m_GamesDir);
    std::cout << "\n";
}

bool ValkyrieEngineApp::Init(const std::string& rootDir) {
    m_RootDir = rootDir;
    m_CoreDir = m_RootDir + "\\Core";
    m_CoreConfigPath = m_CoreDir + "\\config.ini";
    m_EngineLibsDir = m_RootDir + "\\EngineLibs";
    m_FakeInjectDir = m_RootDir + "\\FakeInject";
    m_ScenesDir = m_RootDir + "\\Scenes";
    m_ScriptsDir = m_RootDir + "\\Scripts";
    m_GamesDir = m_RootDir + "\\Games";

    CreateDirectoryA(m_CoreDir.c_str(), nullptr);
    CreateDirectoryA(m_FakeInjectDir.c_str(), nullptr);
    CreateDirectoryA(m_ScenesDir.c_str(), nullptr);
    CreateDirectoryA(m_ScriptsDir.c_str(), nullptr);
    CreateDirectoryA(m_EngineLibsDir.c_str(), nullptr);
    CreateDirectoryA(m_GamesDir.c_str(), nullptr);

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
    std::string cleanName = StripExeExtension(gameNameOrExe);
    if (!cleanName.empty()) {
        m_Settings.projectName = cleanName;
        m_Settings.compiledGameExe = cleanName + ".exe";
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
    std::string dllPath = m_FakeInjectDir + "\\" + dllFileName;
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
    CopyAllFilesInDir(m_CoreDir, outDir + "\\Core");
    CopyAllFilesInDir(m_ScenesDir, outDir + "\\Scenes");
    CopyAllFilesInDir(m_ScriptsDir, outDir + "\\Scripts");
    CopyAllFilesInDir(m_EngineLibsDir, outDir + "\\EngineLibs");
    CopyAllFilesInDir(m_FakeInjectDir, outDir + "\\FakeInject");
    std::cout << "[Export] Skopiowano Core, Scenes, Scripts, EngineLibs i FakeInject dla gry ["
        << m_Settings.compiledGameExe << "] do: " << outDir << "\n";
    return true;
}

void ValkyrieEngineApp::ShowSummary() const {
    std::cout << "\n=== ValkyrieEngine Summary ===\n"
        << "Projekt: " << m_Settings.projectName << "\n"
        << "Skompilowana gra (cel): " << m_Settings.compiledGameExe << "\n"
        << "Folder gier: " << m_GamesDir << "\n"
        << "Wczytane elementy scen: " << m_LoadedEntities.size() << "\n"
        << "Podpiete DLL silnika: " << m_EngineDlls.size() << "\n"
        << "Anty-Cheat: " << (m_Settings.antiCheatEnabled ? "ON" : "OFF") << "\n"
        << "Ban za inject: " << (m_Settings.banOnInject ? "ON" : "OFF") << "\n\n";
}