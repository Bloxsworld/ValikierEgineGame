#include "Engine3D.h"
#include <iostream>
#include <sstream>
#include <string>

int main() {
    SetConsoleTitleA("ValkyrieEngine - Main Runner");

    char exeBuf[MAX_PATH]{};
    GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
    std::string exePath(exeBuf);
    size_t pos = exePath.find_last_of("\\/");
    std::string rootDir = (pos != std::string::npos) ? exePath.substr(0, pos) : ".";

    ValkyrieEngineApp app;
    app.Init(rootDir);
    app.ShowSummary();

    std::cout << "Komendy:\n"
        << "  game <NazwaGry>           - ustaw nazwe skompilowanej gry\n"
        << "  builddll [NazwaDll]       - skompiluj i podepnij DLL silnika (np. builddll ValkyrieEngine3D)\n"
        << "  loaddll <Sciezka.dll>     - podepnij gotowa DLL silnika\n"
        << "  tickdll                   - wykonaj krok wszystkich podpietych DLL silnika\n"
        << "  status                    - podsumowanie projektu, celu gry i scen\n"
        << "  ban on | ban off          - wlacz lub wylacz ban za inject w Core\\config.ini\n"
        << "  inject <Nazwa.dll>        - test z FakeInject\\ (np. ValkierCore.dll lub CoreInit.dll)\n"
        << "  export <FolderDocelowy>   - kopiuj pliki silnika i Core\\ pod gre\n"
        << "  exit                      - zakoncz\n\n";

    std::string line;
    while (true) {
        std::cout << "ValkyrieEngine> ";
        if (!std::getline(std::cin, line)) break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd.empty()) continue;

        if (cmd == "exit") {
            break;
        }
        else if (cmd == "game") {
            std::string gameName;
            if (iss >> gameName) app.SetCompiledGameName(gameName);
            else std::cout << "Uzycie: game <NazwaGry>\n";
        }
        else if (cmd == "builddll") {
            std::string dllBase;
            if (iss >> dllBase) app.CompileAndAttachEngineDll(dllBase);
            else app.CompileAndAttachEngineDll("ValkyrieEngine3D");
        }
        else if (cmd == "loaddll") {
            std::string dllPath;
            if (iss >> dllPath) app.AttachEngineDll(dllPath);
            else std::cout << "Uzycie: loaddll <Sciezka.dll>\n";
        }
        else if (cmd == "tickdll") {
            app.TickEngineDlls(0.1f);
        }
        else if (cmd == "status") {
            app.ShowSummary();
        }
        else if (cmd == "ban") {
            std::string opt;
            if (iss >> opt) {
                if (opt == "on") app.SetBanOnInject(true);
                else if (opt == "off") app.SetBanOnInject(false);
            }
        }
        else if (cmd == "inject") {
            std::string dllName;
            if (iss >> dllName) app.RunFakeInject(dllName);
            else std::cout << "Uzycie: inject <Nazwa.dll>\n";
        }
        else if (cmd == "export") {
            std::string outFolder;
            if (iss >> outFolder) app.ExportGameBundle(outFolder);
            else std::cout << "Uzycie: export <FolderDocelowy>\n";
        }
        else {
            std::cout << "Nieznana komenda.\n";
        }
    }

    return 0;
}