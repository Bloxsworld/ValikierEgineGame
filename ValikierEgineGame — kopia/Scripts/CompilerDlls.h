#pragma once
#include <string>
#include <vector>

struct DllPackageSpec {
    std::string packageName;
    std::string sourceCpp;
    std::string outputDll;
    std::string category = "GameplayPack";
};

class CompilerDlls {
public:
    bool BuildDll(const DllPackageSpec& spec);
    bool BuildPackageBundle(const std::string& bundleName, const std::vector<DllPackageSpec>& specs);
    void ListBuiltPackages() const;

private:
    std::vector<std::string> m_BuiltDlls;
    bool RunBuildCommand(const std::string& cmd);
};