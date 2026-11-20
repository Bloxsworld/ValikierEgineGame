#include "CompilerDlls.h"
#include "Logger3D.h"
#include <windows.h>
#include <fstream>
#include <iostream>

bool CompilerDlls::RunBuildCommand(const std::string& cmd) {
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::string mutableCmd = cmd;
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

bool CompilerDlls::BuildDll(const DllPackageSpec& spec) {
    Logger3D::Instance().Build("CompilerDlls",
        "Building DLL package [" + spec.packageName + "] -> " + spec.outputDll);

    std::ifstream srcCheck(spec.sourceCpp);
    if (!srcCheck.is_open()) {
        std::ofstream gen(spec.sourceCpp);
        if (gen.is_open()) {
            gen << "#include <windows.h>\n"
                << "extern \"C\" __declspec(dllexport) const char* PackInfo() {\n"
                << "    return \"ValkyrieEngine Package: " << spec.packageName << "\";\n"
                << "}\n"
                << "BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID res) {\n"
                << "    (void)hMod; (void)reason; (void)res;\n"
                << "    return TRUE;\n"
                << "}\n";
            Logger3D::Instance().Build("CompilerDlls", "Created default DLL source: " + spec.sourceCpp);
        }
    }

    CreateDirectoryA("Packages", nullptr);

    std::string cmdGpp = "cmd.exe /c g++ -shared -std=c++17 \"" + spec.sourceCpp +
        "\" -o \"" + spec.outputDll + "\"";
    if (RunBuildCommand(cmdGpp)) {
        m_BuiltDlls.push_back(spec.outputDll);
        Logger3D::Instance().Build("CompilerDlls", "DLL compiled (g++): " + spec.outputDll);
        return true;
    }

    std::string cmdMsvc = "cmd.exe /c cl /nologo /LD /EHsc \"" + spec.sourceCpp +
        "\" /Fe:\"" + spec.outputDll + "\"";
    if (RunBuildCommand(cmdMsvc)) {
        m_BuiltDlls.push_back(spec.outputDll);
        Logger3D::Instance().Build("CompilerDlls", "DLL compiled (cl.exe): " + spec.outputDll);
        return true;
    }

    Logger3D::Instance().Warn("CompilerDlls",
        "No g++/cl.exe in PATH. Prepared source for package: " + spec.packageName);
    return false;
}

bool CompilerDlls::BuildPackageBundle(const std::string& bundleName, const std::vector<DllPackageSpec>& specs) {
    CreateDirectoryA("Packages", nullptr);
    std::string manifestPath = "Packages\\" + bundleName + ".vpack";

    Logger3D::Instance().Build("CompilerDlls",
        "Packaging bundle [" + bundleName + "] with " + std::to_string(specs.size()) + " DLL modules");

    int okCount = 0;
    for (const auto& s : specs) {
        if (BuildDll(s)) {
            okCount++;
        }
    }

    std::ofstream manifest(manifestPath);
    if (manifest.is_open()) {
        manifest << "# ValkyrieEngine DLL Package Bundle\n";
        manifest << "BundleName=" << bundleName << "\n";
        manifest << "TotalModules=" << specs.size() << "\n";
        manifest << "CompiledModules=" << okCount << "\n";
        for (const auto& s : specs) {
            manifest << "Module=" << s.packageName << " | " << s.category << " | " << s.outputDll << "\n";
        }
    }

    Logger3D::Instance().Build("CompilerDlls",
        "Bundle manifest saved: " + manifestPath + " (" + std::to_string(okCount) + "/" +
        std::to_string(specs.size()) + " compiled)");
    return true;
}

void CompilerDlls::ListBuiltPackages() const {
    Logger3D::Instance().Info("CompilerDlls", "Built DLLs count: " + std::to_string(m_BuiltDlls.size()));
    for (const auto& dll : m_BuiltDlls) {
        std::cout << "  [DLL] " << dll << "\n";
    }
}