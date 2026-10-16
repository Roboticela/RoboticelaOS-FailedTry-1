#include "Robo/RoboScene.hpp"

#include <stdint.h>

void BuildDevKitPage(Robo::RoboScene& scene);
int BuildDevKitPageStep(Robo::RoboScene& scene, int step);

extern "C" void RoboDevKitBuild(Robo::RoboScene& scene) {
    BuildDevKitPage(scene);
}

extern "C" int RoboDevKitBuildStep(Robo::RoboScene& scene, int step) {
    return BuildDevKitPageStep(scene, step);
}

struct RoboDevKitExports {
    void (*build)(Robo::RoboScene&);
    int (*step)(Robo::RoboScene&, int);
};

/* First bytes of the DevKit image — bootloader stores the load base, host reads this table. */
extern "C" const RoboDevKitExports RoboDevKitTable
    __attribute__((section(".devkit_exports"), aligned(16), used)) = {
        &RoboDevKitBuild,
        &RoboDevKitBuildStep,
};
