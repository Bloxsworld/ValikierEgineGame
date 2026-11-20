#pragma once
#include "Logger3D.h"
#include <iostream>
#include <fstream>
#include <string>

class MadeByScene3D {
public:
    std::string sceneName = "MadeByValkyrieEngine_Intro";
    std::string bannerText = "Made By ValkyrieEngine";
    float bannerPosX = 0.0f;
    float bannerPosY = 3.5f;
    float bannerPosZ = 10.0f;

    void ShowSplashScene() const {
        Logger3D::Instance().Info("MadeByScene3D", "Entering scene: " + sceneName);
        std::cout << "\n==============================================================\n";
        std::cout << "             *** " << bannerText << " ***             \n";
        std::cout << "       3D World | C++ Scripts | Core Anti-Cheat | Packages    \n";
        std::cout << "==============================================================\n\n";
    }

    void ExportToSceneFile(const std::string& sceneFilePath) const {
        std::ofstream out(sceneFilePath, std::ios::app);
        if (out.is_open()) {
            out << "SceneBanner MadeByValkyrieEngine " << bannerPosX << " " << bannerPosY << " " << bannerPosZ << "\n";
            Logger3D::Instance().Info("MadeByScene3D", "Appended MadeByValkyrieEngine banner to " + sceneFilePath);
        }
    }
};