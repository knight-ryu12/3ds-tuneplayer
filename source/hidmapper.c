#include <3ds.h>
#include <stdlib.h>
#include <string.h>
#include "hidmapper.h"

static const HIDBind emptybind[] = {{HIDBINDNULL}};
static const HIDBind* active = emptybind;
static bool mapallocated = false;
static RecursiveLock hidmaplock = {1, 0, 0}; //global, so preinitializing. Wished ctrulib would give an initializer macro like pthread_mutex_t has

Result HIDMapper_SetMapping(const HIDBind* binding, bool copy) {
    if (!binding)
        return MAKERESULT(RL_FATAL, RS_INVALIDARG, RM_APPLICATION, RD_INVALID_POINTER);
    RecursiveLock_Lock(&hidmaplock);
    if (!copy) {
        if (mapallocated)
            free((void*)active);
        active = binding;
        mapallocated = false;
        RecursiveLock_Unlock(&hidmaplock);
        return 0;
    }

    const HIDBind* current = binding;
    size_t count = 0;
    while (current->key || current->func) {
        if (current->func)
            count++;
        current++;
    }

    if (!count) {
        if (mapallocated)
            free((void*)active);
        active = emptybind;
        mapallocated = false;
        RecursiveLock_Unlock(&hidmaplock);
        return 0;
    }

    HIDBind* newactive = (HIDBind*)malloc(sizeof(HIDBind) * (count+1));

    if (!newactive) {
        RecursiveLock_Unlock(&hidmaplock);
        return MAKERESULT(RL_FATAL, RS_OUTOFRESOURCE, RM_APPLICATION, RD_OUT_OF_MEMORY);
    }

    current = binding;
    HIDBind* newcopy = newactive;
    while (current->key || current->func) {
        if (!current->func) {
            current++;
            continue;
        }
        *newcopy = *current;
        newcopy++;
        current++;
    }
    *newcopy = (HIDBind){HIDBINDNULL};

    if (mapallocated)
        free((void*)active);
    active = newactive;
    mapallocated = true;

    RecursiveLock_Unlock(&hidmaplock);
    return 0;
}

Result HIDMapper_ClearMapping() {
    return HIDMapper_SetMapping(emptybind, false);
}

Result HIDMapper_RunFrame() {
    RecursiveLock_Lock(&hidmaplock);

    hidScanInput();
    HIDFrame frame = {hidKeysDown(), hidKeysHeld()};
    const HIDBind* current = active;
    Result res = 0;

    while (current->key || current->func) {
        if (!current->func) {
            current++;
            continue;
        }

        int ret;
        if (!current->key || (current->press ? frame.pressed : frame.held) & current->key)
            ret = current->func(frame, current->arg);

        if (ret) {
            if (ret > 0)
                res = MAKERESULT(RL_INFO, RS_STATUSCHANGED, RM_APPLICATION, RD_CANCEL_REQUESTED);
            else res = MAKERESULT(RL_FATAL, RS_INVALIDSTATE, RM_APPLICATION, RD_CANCEL_REQUESTED);
            break;
        }

        current++;
    }

    RecursiveLock_Unlock(&hidmaplock);
    return res;
}
