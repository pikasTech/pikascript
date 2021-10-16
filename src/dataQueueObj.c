#include "dataQueueObj.h"
#include "BaseObj.h"

QueueObj* New_queueObj() {
    PikaObj* self = New_TinyObj(NULL);
    queueObj_init(self);
    return self;
}

int32_t queueObj_init(QueueObj* self) {
    obj_setInt(self, "top", 0);
    obj_setInt(self, "bottom", 0);
    return 0;
}

int32_t queueObj_pushObj(QueueObj* self, char* className) {
    uint64_t top = obj_getInt(self, "top");
    char topStr[32] = {0};
    sprintf(topStr, "%llu", top);
    /* add top */
    obj_setInt(self, "top", top + 1);
    return obj_newObjByFun(self, topStr, className, New_TinyObj);
}

PikaObj* queueObj_getCurrentObj(QueueObj* self) {
    uint64_t current = obj_getInt(self, "top") - 1;
    char currentStr[32] = {0};
    sprintf(currentStr, "%llu", current);
    return obj_getObj(self, currentStr, 0);
}

PikaObj* queueObj_popObj(QueueObj* self) {
    uint64_t bottom = obj_getInt(self, "bottom");
    char bottomStr[32] = {0};
    sprintf(bottomStr, "%llu", bottom);
    /* add bottom */
    obj_setInt(self, "bottom", bottom + 1);
    PikaObj* res = obj_getObj(self, bottomStr, 0);
    return res;
}

int32_t queueObj_pushInt(QueueObj* self, int val) {
    uint64_t top = obj_getInt(self, "top");
    char topStr[32] = {0};
    sprintf(topStr, "%llu", top);
    /* add top */
    obj_setInt(self, "top", top + 1);
    return obj_setInt(self, topStr, val);
}

int64_t queueObj_popInt(QueueObj* self) {
    uint64_t bottom = obj_getInt(self, "bottom");
    char bottomStr[32] = {0};
    sprintf(bottomStr, "%llu", bottom);
    /* add bottom */
    obj_setInt(self, "bottom", bottom + 1);
    int64_t res = obj_getInt(self, bottomStr);
    obj_removeArg(self, bottomStr);
    return res;
}

int32_t queueObj_pushFloat(QueueObj* self, float val) {
    uint64_t top = obj_getInt(self, "top");
    char topStr[32] = {0};
    sprintf(topStr, "%llu", top);
    /* add top */
    obj_setInt(self, "top", top + 1);
    return obj_setFloat(self, topStr, val);
}

float queueObj_popFloat(QueueObj* self) {
    uint64_t bottom = obj_getInt(self, "bottom");
    char bottomStr[32] = {0};
    sprintf(bottomStr, "%llu", bottom);
    /* add bottom */
    obj_setInt(self, "bottom", bottom + 1);
    float res = obj_getFloat(self, bottomStr);
    obj_removeArg(self, bottomStr);
    return res;
}

int32_t queueObj_pushStr(QueueObj* self, char* str) {
    uint64_t top = obj_getInt(self, "top");
    char topStr[32] = {0};
    sprintf(topStr, "%llu", top);
    /* add top */
    obj_setInt(self, "top", top + 1);
    return obj_setStr(self, topStr, str);
}

char* queueObj_popStr(QueueObj* self) {
    uint64_t bottom = obj_getInt(self, "bottom");
    char bottomStr[32] = {0};
    sprintf(bottomStr, "%llu", bottom);
    /* add bottom */
    obj_setInt(self, "bottom", bottom + 1);
    return obj_getStr(self, bottomStr);
}