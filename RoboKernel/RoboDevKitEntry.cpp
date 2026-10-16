#include "Robo/RoboScene.hpp"

void BuildDevKitPage(Robo::RoboScene& scene);

extern "C" void RoboDevKitBuild(Robo::RoboScene& scene) {
    BuildDevKitPage(scene);
}
