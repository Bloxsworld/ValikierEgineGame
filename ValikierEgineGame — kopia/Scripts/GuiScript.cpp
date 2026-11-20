#include <iostream>
#include <string>

int main() {
    int hp = 100;
    int stamina = 90;
    std::string banner = "Made By ValkyrieEngine";

    std::cout << "+-------------------------------------------+\n";
    std::cout << "| " << banner << "                  |\n";
    std::cout << "| HP: " << hp << " | Stamina: " << stamina << " | GUI: ACTIVE        |\n";
    std::cout << "+-------------------------------------------+\n";
    return 0;
}