#pragma once
#include "Logger3D.h"
#include <windows.h>
#include <fstream>
#include <string>

class ScriptCompiler3D {
public:
    // Tworzy lub kompiluje wskazany plik skryptu .cpp za pomoca cl.exe lub g++
    bool CompileScriptFile(const std::string& cppFilePath, const std::string& outExePath) {
        Logger3D::Instance().Build("ScriptCompiler3D", "Starting live compilation of: " + cppFilePath);

        std::ifstream check(cppFilePath);
        if (!check.is_open()) {
            Logger3D::Instance().Error("ScriptCompiler3D", "Source file not found: " + cppFilePath);
            return false;
        }

        // Proba kompilacji przez g++ (MinGW) albo cl.exe (MSVC)
        std::string cmdGpp = "cmd.exe /c g++ -std=c++17 \"" + cppFilePath + "\" -o \"" + outExePath + "\"";
        if (RunCommandWait(cmdGpp)) {
            Logger3D::Instance().Build("ScriptCompiler3D", "Compiled with g++ -> " + outExePath);
            return true;
        }

        std::string cmdMsvc = "cmd.exe /c cl /nologo /EHsc \"" + cppFilePath + "\" /Fe:\"" + outExePath + "\"";
        if (RunCommandWait(cmdMsvc)) {
            Logger3D::Instance().Build("ScriptCompiler3D", "Compiled with cl.exe -> " + outExePath);
            return true;
        }

        Logger3D::Instance().Warn("ScriptCompiler3D",
            "No g++ or cl.exe in PATH. Saved/verified script source: " + cppFilePath);
        return false;
    }

    bool CreateSampleMovableScript(const std::string& filePath) {
        std::ofstream out(filePath);
        if (!out.is_open()) return false;

        out << "#include <iostream>\n"
            << "int main() {\n"
            << "    std::cout << \"[LiveCompiledScript] Dynamic 3D script executed successfully!\\n\";\n"
            << "    return 0;\n"
            << "}\n";

        Logger3D::Instance().Build("ScriptCompiler3D", "Generated live script source: " + filePath);
        return true;
    }

private:
    bool RunCommandWait(const std::string& commandLine) {
        STARTUPINFOA si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        std::string mutableCmd = commandLine;
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

        WaitForSingleObject(pi.hProcess, 15000);
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return exitCode == 0;
    }
};