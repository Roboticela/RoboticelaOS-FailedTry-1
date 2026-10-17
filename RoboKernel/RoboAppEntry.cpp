#include "Robo/RoboScene.hpp"

void BuildTestAppPage(Robo::RoboScene& scene);
int BuildTestAppPageStep(Robo::RoboScene& scene, int step);

extern "C" void RoboAppBuild(Robo::RoboScene& scene) {
    BuildTestAppPage(scene);
}

extern "C" int RoboAppBuildStep(Robo::RoboScene& scene, int step) {
    return BuildTestAppPageStep(scene, step);
}

struct RoboAppExports {
    void (*build)(Robo::RoboScene&);
    int (*step)(Robo::RoboScene&, int);
};

/* First bytes of the app image — host reads this table at the load base. */
extern "C" const RoboAppExports RoboAppTable
    __attribute__((section(".robo_app_exports"), used)) = {
        &RoboAppBuild,
        &RoboAppBuildStep,
};
