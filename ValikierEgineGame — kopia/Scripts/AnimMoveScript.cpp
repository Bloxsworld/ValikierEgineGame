#include <iostream>
#include <cmath>
#include <string>

int main() {
    std::cout << "[AnimMoveScript] Test animacji ruszania postaci 3D\n";
    const char* states[] = { "Idle", "Walk", "Run", "Jump", "Land" };
    float phase = 0.0f;

    for (int i = 0; i < 5; ++i) {
        phase += 1.57f;
        float legSwing = std::sin(phase) * 35.0f;
        float bob = std::fabs(std::cos(phase)) * 0.1f;
        std::cout << "  Stan: " << states[i]
            << " | wychylenie nogi: " << legSwing
            << " | bob: " << bob << "\n";
    }
    return 0;
}