#include "PikaStdData_List.h"
#include "BaseObj.h"
#include "PikaObj.h"
#include "PikaStdData_Tuple.h"
#include "dataStrs.h"

void PikaStdData_List_append(PikaObj* self, Arg* arg) {
    __vm_List_append(self, arg);
}

void PikaStdData_List_set(PikaObj* self, Arg* arg, int i) {
    PikaList* list = obj_getPtr(self, "list");
    if (PIKA_RES_OK != list_setArg(list, i, arg)) {
        obj_setErrorCode(self, 1);
        obj_setSysOut(self, "[error]: index exceeded lengh of list.");
    }
}

void PikaStdData_List___set__(PikaObj* self, Arg* __key, Arg* __val) {
    PikaStdData_List_set(self, obj_getArg(self, "__val"),
                         obj_getInt(self, "__key"));
}


void PikaStdData_List___init__(PikaObj* self) {
    __vm_List___init__(self);
}

char* PikaStdLib_SysObj_str(PikaObj* self, Arg* arg);
char* PikaStdData_List___str__(PikaObj* self) {
    Arg* str_arg = arg_newStr("[");
    PikaList* list = obj_getPtr(self, "list");

    int i = 0;
    while (PIKA_TRUE) {
        Arg* item = list_getArg(list, i);
        if (NULL == item) {
            break;
        }
        if (i != 0) {
            str_arg = arg_strAppend(str_arg, ", ");
        }
        char* item_str = PikaStdLib_SysObj_str(self, item);
        str_arg = arg_strAppend(str_arg, item_str);
        i++;
    }

    str_arg = arg_strAppend(str_arg, "]");
    obj_setStr(self, "_buf", arg_getStr(str_arg));
    arg_deinit(str_arg);
    return obj_getStr(self, "_buf");
}
