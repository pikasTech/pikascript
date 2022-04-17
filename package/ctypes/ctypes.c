#include "ctypes_Test.h"
#include "ctypes_c_uint.h"
#include "ctypes_c_wchar_p.h"
#include "ctypes_utils.h"

void ctypes_c_uint___init__(PikaObj* self, int value) {
    obj_setInt(self, "value", value);
}

void ctypes_c_wchar_p___init__(PikaObj* self, char* value) {
    obj_setStr(self, "value", value);
}

int ctypes_Test_add(PikaObj* self, PikaObj* c_uint1, PikaObj* c_uint2) {
    return obj_getInt(c_uint1, "value") + obj_getInt(c_uint2, "value");
}

int ctypes_Test_dc_cpuapdu_hex(PikaObj* self,
                               PikaObj* rcvbuf,
                               PikaObj* rlen,
                               char* sendbuf,
                               int slen) {
    printf("input: slen = %d, sendbuf = %s\n", slen, sendbuf);
    ctypesUtils_setInt(rlen, 5);
    ctypesUtils_setStr(rcvbuf, "testout");
    return 0;
}
