#include "RoboPermit.hpp"

#include "RoboFS.hpp"

namespace {

constexpr uint32_t ROBO_PERMIT_LIMIT = 32u;
constexpr uint32_t ROBO_PERMIT_APP = 32u;
constexpr uint32_t ROBO_PERMIT_PATH = 80u;

struct RoboGrant {
    char app[ROBO_PERMIT_APP];
    char path[ROBO_PERMIT_PATH];
    uint32_t scope;
    uint32_t rights;
    uint32_t disk;
    uint32_t used;
};

static_assert(sizeof(RoboGrant) == 128u, "A permission record is one packed line");

RoboGrant gGrants[ROBO_PERMIT_LIMIT];

bool RoboSame(const char* left, const char* right, uint32_t limit) {
    for (uint32_t index = 0; index < limit; ++index) {
        if (left[index] != right[index]) {
            return false;
        }
        if (left[index] == 0) {
            return true;
        }
    }
    return false;
}

bool RoboCopy(char* dest, uint32_t capacity, const char* text) {
    if (text == nullptr) {
        text = "";
    }
    uint32_t length = 0;
    while (text[length] != 0) {
        length += 1u;
        if (length >= capacity) {
            return false;
        }
    }
    for (uint32_t index = 0; index < capacity; ++index) {
        dest[index] = 0;
    }
    for (uint32_t index = 0; index < length; ++index) {
        dest[index] = text[index];
    }
    return true;
}

bool RoboAppName(const char* app) {
    if (app == nullptr || app[0] == 0) {
        return false;
    }
    for (uint32_t index = 0; app[index] != 0; ++index) {
        if (app[index] == '/' || app[index] == '\\' || index >= ROBO_PERMIT_APP - 1u) {
            return false;
        }
    }
    return true;
}

bool RoboGrantPath(uint32_t scope, const char* path) {
    if (scope == ROBO_SCOPE_DISK || scope == ROBO_SCOPE_STORAGE) {
        return path == nullptr || path[0] == 0;
    }
    if (scope != ROBO_SCOPE_FILE && scope != ROBO_SCOPE_FOLDER) {
        return false;
    }
    if (path == nullptr || path[0] == 0 || path[0] == '/') {
        return false;
    }
    uint32_t length = 0;
    while (path[length] != 0) {
        if (path[length] == '\\') {
            return false;
        }
        length += 1u;
        if (length >= ROBO_PERMIT_PATH) {
            return false;
        }
    }
    return path[length - 1u] != '/';
}

bool RoboCovers(const RoboGrant* grant, const char* path) {
    if (grant->used == 0) {
        return false;
    }
    if (grant->scope == ROBO_SCOPE_STORAGE) {
        return true;
    }
    if (grant->scope == ROBO_SCOPE_DISK) {
        return grant->disk == ROBO_PERMIT_DISK_MOUNTED;
    }
    if (path == nullptr) {
        return false;
    }
    if (grant->scope == ROBO_SCOPE_FILE) {
        return RoboSame(grant->path, path, ROBO_PERMIT_PATH);
    }
    if (grant->scope != ROBO_SCOPE_FOLDER) {
        return false;
    }
    for (uint32_t index = 0; index < ROBO_PERMIT_PATH; ++index) {
        char grantChar = grant->path[index];
        char pathChar = path[index];
        if (grantChar == 0) {
            return pathChar == 0 || pathChar == '/';
        }
        if (grantChar != pathChar) {
            return false;
        }
    }
    return false;
}

int RoboPermitStore(void) {
    return RoboFsCommit("System/Permissions", reinterpret_cast<const uint8_t*>(gGrants), sizeof(gGrants));
}

int RoboFind(const char* app, uint32_t scope, const char* path, uint32_t disk) {
    for (uint32_t index = 0; index < ROBO_PERMIT_LIMIT; ++index) {
        RoboGrant* grant = &gGrants[index];
        if (grant->used == 0 || grant->scope != scope || grant->disk != disk) {
            continue;
        }
        if (!RoboSame(grant->app, app, ROBO_PERMIT_APP)) {
            continue;
        }
        const char* grantPath = grant->path;
        const char* wanted = path == nullptr ? "" : path;
        if (RoboSame(grantPath, wanted, ROBO_PERMIT_PATH)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

}  // namespace

void RoboPermitStart(void) {
    for (uint32_t index = 0; index < ROBO_PERMIT_LIMIT; ++index) {
        gGrants[index].used = 0;
        gGrants[index].app[0] = 0;
        gGrants[index].path[0] = 0;
    }
    uint8_t file[sizeof(gGrants)];
    uint32_t size = 0;
    if (!RoboFsReadFile("System/Permissions", file, sizeof(file), &size) || size < sizeof(RoboGrant)) {
        return;
    }
    uint32_t count = size / sizeof(RoboGrant);
    if (count > ROBO_PERMIT_LIMIT) {
        count = ROBO_PERMIT_LIMIT;
    }
    for (uint32_t index = 0; index < count; ++index) {
        const uint8_t* record = file + index * sizeof(RoboGrant);
        RoboGrant* grant = &gGrants[index];
        for (uint32_t byte = 0; byte < sizeof(RoboGrant); ++byte) {
            reinterpret_cast<uint8_t*>(grant)[byte] = record[byte];
        }
        if (grant->used != 1u || grant->app[0] == 0) {
            grant->used = 0;
        }
    }
}

int RoboPermitGrant(const char* app, uint32_t scope, const char* path, uint32_t disk, uint32_t rights) {
    if (!RoboAppName(app) || !RoboGrantPath(scope, path) || rights == 0 || (rights & ~ROBO_PERMIT_ALL) != 0) {
        return ROBO_FS_DENIED;
    }
    int slot = RoboFind(app, scope, path, disk);
    if (slot < 0) {
        for (uint32_t index = 0; index < ROBO_PERMIT_LIMIT; ++index) {
            if (gGrants[index].used == 0) {
                slot = static_cast<int>(index);
                break;
            }
        }
    }
    if (slot < 0) {
        return ROBO_FS_FULL;
    }
    RoboGrant saved = gGrants[slot];
    RoboGrant* grant = &gGrants[slot];
    if (!RoboCopy(grant->app, ROBO_PERMIT_APP, app) || !RoboCopy(grant->path, ROBO_PERMIT_PATH, path == nullptr ? "" : path)) {
        *grant = saved;
        return ROBO_FS_DENIED;
    }
    grant->scope = scope;
    grant->rights = rights;
    grant->disk = disk;
    grant->used = 1;
    int stored = RoboPermitStore();
    if (stored != ROBO_FS_OK) {
        *grant = saved;
    }
    return stored;
}

int RoboPermitRevoke(const char* app, uint32_t scope, const char* path, uint32_t disk) {
    if (!RoboAppName(app)) {
        return ROBO_FS_DENIED;
    }
    int slot = RoboFind(app, scope, path, disk);
    if (slot < 0) {
        return ROBO_FS_MISSING;
    }
    RoboGrant saved = gGrants[slot];
    for (uint32_t byte = 0; byte < sizeof(RoboGrant); ++byte) {
        reinterpret_cast<uint8_t*>(&gGrants[slot])[byte] = 0;
    }
    int stored = RoboPermitStore();
    if (stored != ROBO_FS_OK) {
        gGrants[slot] = saved;
    }
    return stored;
}

bool RoboPermitAllows(const char* app, uint32_t right, const char* path) {
    if (!RoboAppName(app) || right == 0 || path == nullptr || path[0] == 0) {
        return false;
    }
    for (uint32_t index = 0; index < ROBO_PERMIT_LIMIT; ++index) {
        RoboGrant* grant = &gGrants[index];
        if (grant->used == 0 || !RoboSame(grant->app, app, ROBO_PERMIT_APP)) {
            continue;
        }
        if ((grant->rights & right) == right && RoboCovers(grant, path)) {
            return true;
        }
    }
    return false;
}

uint32_t RoboPermitRights(const char* app, const char* path) {
    uint32_t rights = 0;
    if (!RoboAppName(app) || path == nullptr) {
        return 0;
    }
    for (uint32_t index = 0; index < ROBO_PERMIT_LIMIT; ++index) {
        RoboGrant* grant = &gGrants[index];
        if (grant->used == 0 || !RoboSame(grant->app, app, ROBO_PERMIT_APP)) {
            continue;
        }
        if (RoboCovers(grant, path)) {
            rights |= grant->rights;
        }
    }
    return rights;
}
