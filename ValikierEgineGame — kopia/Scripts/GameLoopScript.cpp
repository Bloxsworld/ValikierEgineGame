#include <iostream>
#include <fstream>
#include <string>

static void WriteLog(const std::string& msg) {
    std::ofstream log("Scripts\\game_script.log", std::ios::app);
    if (log.is_open()) {
        log << "[GameLoopScript] " << msg << "\n";
    }
    std::cout << "[GameLoopScript] " << msg << "\n";
}

int main() {
    WriteLog("Start petli gry 3D");
    float posZ = 0.0f;
    for (int step = 1; step <= 5; ++step) {
        posZ += 1.25f;
        WriteLog("Krok " + std::to_string(step) + " -> pozycja Z = " + std::to_string(posZ));
    }
    WriteLog("Koniec testu petli gry");
    return 0;
}