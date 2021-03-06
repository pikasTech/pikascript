/*
 * This file is part of the PikaScript project.
 * http://github.com/pikastech/pikascript
 *
 * MIT License
 *
 * Copyright (c) 2021 lyon 李昂 liang6516@outlook.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "PikaVM.h"
#include "BaseObj.h"
#include "PikaCompiler.h"
#include "PikaObj.h"
#include "PikaParser.h"
#include "PikaPlatform.h"
#include "dataStrs.h"

/* head declear start */
static uint8_t VMState_getInputArgNum(VMState* vs);
static VMParameters* __pikaVM_runByteCodeFrameWithState(
    PikaObj* self,
    VMParameters* locals,
    VMParameters* globals,
    ByteCodeFrame* bytecode_frame,
    uint16_t pc,
    TryInfo* try_info);

/* head declear end */

static InstructUnit* VMState_getInstructNow(VMState* vs) {
    return instructArray_getByOffset(&(vs->bytecode_frame->instruct_array),
                                     vs->pc);
}

static void VMState_setErrorCode(VMState* vs, uint8_t error_code) {
    vs->error_code = error_code;
}

static InstructUnit* VMState_getInstructWithOffset(VMState* vs,
                                                   int32_t offset) {
    return instructArray_getByOffset(&(vs->bytecode_frame->instruct_array),
                                     vs->pc + offset);
}

static int VMState_getBlockDeepthNow(VMState* vs) {
    /* support run byteCode */
    InstructUnit* ins_unit = VMState_getInstructNow(vs);
    return instructUnit_getBlockDeepth(ins_unit);
}

static char* VMState_getConstWithInstructUnit(VMState* vs,
                                              InstructUnit* ins_unit) {
    return constPool_getByOffset(&(vs->bytecode_frame->const_pool),
                                 instructUnit_getConstPoolIndex(ins_unit));
}

static int32_t VMState_getAddrOffsetOfJmpBack(VMState* vs) {
    int offset = 0;
    int loop_deepth = -1;

    /* find loop deepth */
    while (1) {
        offset -= instructUnit_getSize(ins_unit_now);
        InstructUnit* ins_unit_now = VMState_getInstructWithOffset(vs, offset);
        uint16_t invoke_deepth = instructUnit_getInvokeDeepth(ins_unit_now);
        enum Instruct ins = instructUnit_getInstruct(ins_unit_now);
        char* data = VMState_getConstWithInstructUnit(vs, ins_unit_now);
        if ((0 == invoke_deepth) && (JEZ == ins) && strEqu(data, "2")) {
            loop_deepth = instructUnit_getBlockDeepth(ins_unit_now);
            break;
        }
    }

    offset = 0;
    while (1) {
        offset += instructUnit_getSize(ins_unit_now);
        InstructUnit* ins_unit_now = VMState_getInstructWithOffset(vs, offset);
        enum Instruct ins = instructUnit_getInstruct(ins_unit_now);
        char* data = VMState_getConstWithInstructUnit(vs, ins_unit_now);
        int block_deepth_now = instructUnit_getBlockDeepth(ins_unit_now);
        if ((block_deepth_now == loop_deepth) && (JMP == ins) &&
            strEqu(data, "-1")) {
            return offset;
        }
    }
}

static size_t VMState_getInstructArraySize(VMState* vs) {
    return instructArray_getSize(&(vs->bytecode_frame->instruct_array));
}

static int32_t VMState_getAddrOffsetFromJmp(VMState* vs) {
    int offset = 0;
    /* run byte Code */
    InstructUnit* this_ins_unit = VMState_getInstructNow(vs);
    int thisBlockDeepth = instructUnit_getBlockDeepth(this_ins_unit);
    int8_t blockNum = 0;

    if (vs->jmp > 0) {
        offset = 0;
        while (1) {
            offset += instructUnit_getSize();
            /* reach the end */
            if (vs->pc + offset >= (int)VMState_getInstructArraySize(vs)) {
                break;
            }
            this_ins_unit = VMState_getInstructWithOffset(vs, offset);
            if (instructUnit_getIsNewLine(this_ins_unit)) {
                uint8_t blockDeepth =
                    instructUnit_getBlockDeepth(this_ins_unit);
                if (blockDeepth <= thisBlockDeepth) {
                    blockNum++;
                }
            }
            if (blockNum >= vs->jmp) {
                break;
            }
        }
    }
    if (vs->jmp < 0) {
        while (1) {
            offset -= instructUnit_getSize();
            this_ins_unit = VMState_getInstructWithOffset(vs, offset);
            if (instructUnit_getIsNewLine(this_ins_unit)) {
                uint8_t blockDeepth =
                    instructUnit_getBlockDeepth(this_ins_unit);
                if (blockDeepth == thisBlockDeepth) {
                    blockNum--;
                }
            }
            if (blockNum <= vs->jmp) {
                break;
            }
        }
    }
    return offset;
}

static int32_t VMState_getAddrOffsetOfBreak(VMState* vs) {
    int32_t offset = VMState_getAddrOffsetOfJmpBack(vs);
    /* byteCode */
    offset += instructUnit_getSize();
    return offset;
}

static int32_t VMState_getAddrOffsetOfRaise(VMState* vs) {
    int offset = 0;
    InstructUnit* ins_unit_now = VMState_getInstructNow(vs);
    while (1) {
        offset += instructUnit_getSize(ins_unit_now);
        ins_unit_now = VMState_getInstructWithOffset(vs, offset);
        enum Instruct ins = instructUnit_getInstruct(ins_unit_now);
        if ((NTR == ins)) {
            return offset;
        }
    }
}

static int32_t VMState_getAddrOffsetOfContinue(VMState* vs) {
    int32_t offset = VMState_getAddrOffsetOfJmpBack(vs);
    /* byteCode */
    return offset;
}

typedef Arg* (*VM_instruct_handler)(PikaObj* self, VMState* vs, char* data);

static Arg* VM_instruction_handler_NON(PikaObj* self, VMState* vs, char* data) {
    return NULL;
}

Arg* __vm_get(PikaObj* self, Arg* key, Arg* obj) {
    ArgType obj_type = arg_getType(obj);
    int index = 0;
    if (ARG_TYPE_INT == arg_getType(key)) {
        index = arg_getInt(key);
    }
    if (ARG_TYPE_STRING == obj_type) {
        char* str_pyload = arg_getStr(obj);
        char char_buff[] = " ";
        if (index < 0) {
            index = strGetSize(str_pyload) + index;
        }
        char_buff[0] = str_pyload[index];
        return arg_newStr(char_buff);
    }
    if (ARG_TYPE_BYTES == obj_type) {
        uint8_t* bytes_pyload = arg_getBytes(obj);
        uint8_t byte_buff[] = " ";
        if (index < 0) {
            index = arg_getBytesSize(obj) + index;
        }
        byte_buff[0] = bytes_pyload[index];
        return arg_newBytes(byte_buff, 1);
    }
    if (argType_isObject(obj_type)) {
        PikaObj* arg_obj = arg_getPtr(obj);
        obj_setArg(arg_obj, "__key", key);
        // pikaVM_runAsm(arg_obj,
        //               "B0\n"
        //               "1 REF __key\n"
        //               "0 RUN __get__\n"
        //               "0 OUT __res\n");
        const uint8_t bytes[] = {
            0x0c, 0x00, /* instruct array size */
            0x10, 0x81, 0x01, 0x00, 0x00, 0x02, 0x07, 0x00, 0x00, 0x04, 0x0f,
            0x00,
            /* instruct array */
            0x15, 0x00, /* const pool size */
            0x00, 0x5f, 0x5f, 0x6b, 0x65, 0x79, 0x00, 0x5f, 0x5f, 0x67, 0x65,
            0x74, 0x5f, 0x5f, 0x00, 0x5f, 0x5f, 0x72, 0x65, 0x73,
            0x00, /* const pool */
        };
        pikaVM_runByteCode(arg_obj, (uint8_t*)bytes);
        return arg_copy(args_getArg(arg_obj->list, "__res"));
    }
    return arg_newNull();
}

Arg* __vm_slice(PikaObj* self, Arg* end, Arg* obj, Arg* start, int step) {
#if PIKA_SYNTAX_SLICE_ENABLE
    /* No interger index only support __get__ */
    if (!(arg_getType(start) == ARG_TYPE_INT &&
          arg_getType(end) == ARG_TYPE_INT)) {
        return __vm_get(self, start, obj);
    }

    int start_i = arg_getInt(start);
    int end_i = arg_getInt(end);

    /* __slice__ is equal to __get__ */
    if (end_i - start_i == 1) {
        return __vm_get(self, start, obj);
    }

    if (ARG_TYPE_STRING == arg_getType(obj)) {
        size_t len = strGetSize(arg_getStr(obj));
        if (start_i < 0) {
            start_i += len;
        }
        if (end_i < 0) {
            end_i += len + 1;
        }
        Arg* sliced_arg = arg_newStr("");
        for (int i = start_i; i < end_i; i++) {
            Arg* i_arg = arg_newInt(i);
            Arg* item_arg = __vm_get(self, i_arg, obj);
            sliced_arg = arg_strAppend(sliced_arg, arg_getStr(item_arg));
            arg_deinit(item_arg);
            arg_deinit(i_arg);
        }
        return sliced_arg;
    }

    if (ARG_TYPE_BYTES == arg_getType(obj)) {
        size_t len = arg_getBytesSize(obj);
        if (start_i < 0) {
            start_i += len;
        }
        if (end_i < 0) {
            end_i += len + 1;
        }
        Arg* sliced_arg = arg_newBytes(NULL, 0);
        for (int i = start_i; i < end_i; i++) {
            Arg* i_arg = arg_newInt(i);
            Arg* item_arg = __vm_get(self, i_arg, obj);
            uint8_t* bytes_origin = arg_getBytes(sliced_arg);
            size_t size_origin = arg_getBytesSize(sliced_arg);
            Arg* sliced_arg_new = arg_newBytes(NULL, size_origin + 1);
            __platform_memcpy(arg_getBytes(sliced_arg_new), bytes_origin,
                              size_origin);
            __platform_memcpy(arg_getBytes(sliced_arg_new) + size_origin,
                              arg_getBytes(item_arg), 1);
            arg_deinit(sliced_arg);
            sliced_arg = sliced_arg_new;
            arg_deinit(item_arg);
            arg_deinit(i_arg);
        }
        return sliced_arg;
    }
    return arg_newNull();
#else
    return __vm_get(self, start, obj);
#endif
}

static Arg* VM_instruction_handler_SLC(PikaObj* self, VMState* vs, char* data) {
#if PIKA_SYNTAX_SLICE_ENABLE
    int arg_num_input = VMState_getInputArgNum(vs);
    if (arg_num_input < 2) {
        return arg_newNull();
    }
    if (arg_num_input == 2) {
        Arg* key = stack_popArg(&vs->stack);
        Arg* obj = stack_popArg(&vs->stack);
        Arg* res = __vm_get(self, key, obj);
        arg_deinit(key);
        arg_deinit(obj);
        return res;
    }
    if (arg_num_input == 3) {
        Arg* end = stack_popArg(&vs->stack);
        Arg* start = stack_popArg(&vs->stack);
        Arg* obj = stack_popArg(&vs->stack);
        Arg* res = __vm_slice(self, end, obj, start, 1);
        arg_deinit(end);
        arg_deinit(obj);
        arg_deinit(start);
        return res;
    }
#endif
    return arg_newNull();
}

static Arg* VM_instruction_handler_TRY(PikaObj* self, VMState* vs, char* data) {
    pika_assert(NULL != vs->try_info);
    vs->try_info->try_state = TRY_STATE_TOP;
    return NULL;
}

static Arg* VM_instruction_handler_NTR(PikaObj* self, VMState* vs, char* data) {
    vs->try_info->try_state = TRY_STATE_NONE;
    return NULL;
}

static Arg* VM_instruction_handler_NEW(PikaObj* self, VMState* vs, char* data) {
    Arg* origin_arg = obj_getArg(vs->locals, data);
    Arg* new_arg = arg_copy(origin_arg);
    origin_arg = arg_setType(origin_arg, ARG_TYPE_OBJECT);
    arg_setType(new_arg, ARG_TYPE_OBJECT_NEW);
    return new_arg;
}

static Arg* VM_instruction_handler_REF(PikaObj* self, VMState* vs, char* data) {
    if (strEqu(data, (char*)"True")) {
        return arg_newInt(1);
    }
    if (strEqu(data, (char*)"False")) {
        return arg_newInt(0);
    }
    if (strEqu(data, (char*)"None")) {
        return arg_newNull();
    }
    if (strEqu(data, (char*)"RuntimeError")) {
        return arg_newInt(PIKA_RES_ERR_RUNTIME_ERROR);
    }
    Arg* arg = NULL;
    if (data[0] == '.') {
        /* find host from stack */
        Arg* host_obj = stack_popArg(&(vs->stack));
        if (argType_isObject(arg_getType(host_obj))) {
            arg = arg_copy(obj_getArg(arg_getPtr(host_obj), data + 1));
        }
        arg_deinit(host_obj);
    } else {
        /* find in local list first */
        arg = arg_copy(obj_getArg(vs->locals, data));
        if (NULL == arg) {
            /* find in global list second */
            arg = arg_copy(obj_getArg(vs->globals, data));
        }
    }

    if (NULL == arg) {
        VMState_setErrorCode(vs, PIKA_RES_ERR_ARG_NO_FOUND);
        __platform_printf("NameError: name '%s' is not defined\r\n", data);
    }
    return arg;
}

static Arg* VM_instruction_handler_GER(PikaObj* self, VMState* vs, char* data) {
    PIKA_RES err = (PIKA_RES)vs->try_error_code;
    Arg* err_arg = arg_newInt(err);
    return err_arg;
}

static int32_t __foreach_handler_deinitTuple(Arg* argEach, Args* context) {
    if (arg_getType(argEach) == ARG_TYPE_TUPLE) {
        PikaTuple* tuple = arg_getPtr(argEach);
        tuple_deinit(tuple);
    }
    return PIKA_RES_OK;
}

Arg* __obj_runMethodArgWithState(PikaObj* self,
                                 PikaObj* method_args_obj,
                                 Arg* method_arg,
                                 TryInfo* try_state) {
    pika_assert(NULL != try_state);
    Arg* return_arg = NULL;
    /* get method Ptr */
    Method method_ptr = methodArg_getPtr(method_arg);
    /* get method type list */
    ArgType method_type = arg_getType(method_arg);
    /* error */
    if (ARG_TYPE_NONE == method_type) {
        return NULL;
    }
    ByteCodeFrame* method_bytecodeFrame =
        methodArg_getBytecodeFrame(method_arg);
    PikaObj* method_context = methodArg_getDefContext(method_arg);
    if (NULL != method_context) {
        self = method_context;
    }
    obj_setErrorCode(self, PIKA_RES_OK);

    /* run method */
    if (method_type == ARG_TYPE_METHOD_NATIVE) {
        /* native method */
        method_ptr(self, method_args_obj->list);
        /* get method return */
        return_arg =
            arg_copy(args_getArg(method_args_obj->list, (char*)"return"));
    } else if (method_type == ARG_TYPE_METHOD_NATIVE_CONSTRUCTOR) {
        /* native method */
        method_ptr(self, method_args_obj->list);
        /* get method return */
        return_arg =
            arg_copy(args_getArg(method_args_obj->list, (char*)"return"));
    } else {
        /* static method and object method */
        /* byteCode */
        uintptr_t insturctArray_start = (uintptr_t)instructArray_getByOffset(
            &(method_bytecodeFrame->instruct_array), 0);
        uint16_t pc = (uintptr_t)method_ptr - insturctArray_start;
        method_args_obj = __pikaVM_runByteCodeFrameWithState(
            self, method_args_obj, self, method_bytecodeFrame, pc, try_state);

        /* get method return */
        return_arg =
            arg_copy(args_getArg(method_args_obj->list, (char*)"return"));
    }
    args_foreach(method_args_obj->list, __foreach_handler_deinitTuple, NULL);
    return return_arg;
}

Arg* obj_runMethodArg(PikaObj* self,
                      PikaObj* method_args_obj,
                      Arg* method_arg) {
    TryInfo try_info = {.try_state = TRY_STATE_NONE,
                        .try_result = TRY_RESULT_NONE};
    return __obj_runMethodArgWithState(self, method_args_obj, method_arg,
                                       &try_info);
}

static int VMState_loadArgsFromMethodArg(VMState* vs,
                                         PikaObj* method_host_obj,
                                         Args* args,
                                         Arg* method_arg,
                                         char* method_name,
                                         int arg_num_used) {
    Args buffs = {0};
    uint8_t arg_num_dec = 0;
    PIKA_BOOL is_variable = PIKA_FALSE;
    PIKA_BOOL is_get_variable_arg = PIKA_FALSE;
    uint8_t arg_num = 0;
    ArgType method_type = ARG_TYPE_UNDEF;
    uint8_t arg_num_input = 0;
    PikaTuple* tuple = NULL;
    char* variable_tuple_name = NULL;
    char* type_list_buff = NULL;
    int variable_arg_start = 0;

    /* get method type list */
    char* type_list = methodArg_getTypeList(method_arg, &buffs);
    if (NULL == type_list) {
        goto exit;
    }
    method_type = arg_getType(method_arg);

    /* check variable */
    if (strIsContain(type_list, '*')) {
        is_variable = PIKA_TRUE;
    }

    /* get arg_num_dec */
    if (strEqu("", type_list)) {
        arg_num_dec = 0;
    } else {
        arg_num_dec = strCountSign(type_list, ',') + 1;
    }
    if (method_type == ARG_TYPE_METHOD_OBJECT) {
        /* delete the 'self' */
        arg_num_dec--;
    }
    arg_num_input = VMState_getInputArgNum(vs);

    /* check arg num */
    if (method_type == ARG_TYPE_METHOD_NATIVE_CONSTRUCTOR ||
        method_type == ARG_TYPE_METHOD_CONSTRUCTOR ||
        is_variable == PIKA_TRUE) {
        /* skip for constrctor */
        /* skip for variable args */
    } else {
        /* check arg num decleard and input */
        if (arg_num_dec != arg_num_input - arg_num_used) {
            VMState_setErrorCode(vs, PIKA_RES_ERR_INVALID_PARAM);
            __platform_printf(
                "TypeError: %s() takes %d positional argument but %d were "
                "given\r\n",
                method_name, arg_num_dec, arg_num_input);
            goto exit;
        }
    }

    if (PIKA_TRUE == is_variable) {
        arg_num = arg_num_input;
    } else {
        arg_num = arg_num_dec;
    }

    /* get variable tuple name */
    type_list_buff = strsCopy(&buffs, type_list);
    variable_arg_start = 0;
    for (int i = 0; i < arg_num_dec; i++) {
        char* arg_def = strPopLastToken(type_list_buff, ',');
        if (strIsStartWith(arg_def, "*")) {
            /* skip the '*' */
            variable_tuple_name = arg_def + 1;
            variable_arg_start = arg_num_dec - i - 1;
            is_get_variable_arg = PIKA_TRUE;
            break;
        }
    }

    /* found variable arg */
    if (PIKA_TRUE == is_get_variable_arg) {
        tuple = New_tuple();
        strPopLastToken(type_list, ',');
    }

    /* load pars */
    for (int i = 0; i < arg_num; i++) {
        char* arg_name = NULL;
        if (arg_num - i <= variable_arg_start) {
            is_get_variable_arg = PIKA_FALSE;
        }
        if (PIKA_FALSE == is_get_variable_arg) {
            char* arg_def = strPopLastToken(type_list, ',');
            strPopLastToken(arg_def, ':');
            arg_name = arg_def;
        } else {
            /* clear the variable arg name */
            arg_name = strsCopy(&buffs, "");
        }
        Arg* call_arg = stack_popArg(&(vs->stack));
        call_arg = arg_setName(call_arg, arg_name);
        /* load the variable arg */
        if (PIKA_TRUE == is_get_variable_arg) {
            list_append(&tuple->super, call_arg);
            /* the append would copy the arg */
            arg_deinit(call_arg);
        } else {
            /* load normal arg */
            args_setArg(args, call_arg);
        }
    }

    if (PIKA_TRUE == is_variable) {
        /* resort the tuple */
        PikaTuple* tuple_sorted = New_tuple();
        if (NULL != tuple) {
            int tuple_size = tuple_getSize(tuple);
            for (int i = 0; i < tuple_size; i++) {
                Arg* arg = tuple_getArg(tuple, tuple_size - 1 - i);
                list_append(&(tuple_sorted->super), arg);
            }
            tuple_deinit(tuple);
        }
        /* load variable tuple */
        args_setPtrWithType(args, variable_tuple_name, ARG_TYPE_TUPLE,
                            tuple_sorted);
    }

    /* load 'self' as the first arg when call object method */
    if (method_type == ARG_TYPE_METHOD_OBJECT) {
        Arg* call_arg = arg_setRef(NULL, "self", method_host_obj);
        args_setArg(args, call_arg);
    }
exit:
    strsDeinit(&buffs);
    return arg_num;
}

void __vm_List_append(PikaObj* self, Arg* arg) {
    PikaList* list = obj_getPtr(self, "list");
    list_append(list, arg);
}

void __vm_List___init__(PikaObj* self) {
    if (!obj_isArgExist(self, "list")) {
        PikaList* list = New_list();
        obj_setPtr(self, "list", list);
    }
}

#if PIKA_BUILTIN_STRUCT_ENABLE
PikaObj* New_PikaStdData_List(Args* args);
PikaObj* New_PikaStdData_Tuple(Args* args);
#endif

static Arg* _vm_create_list_or_tuple(PikaObj* self,
                                     VMState* vs,
                                     PIKA_BOOL is_list) {
#if PIKA_BUILTIN_STRUCT_ENABLE
    NewFun constructor = is_list ? New_PikaStdData_List : New_PikaStdData_Tuple;
    uint8_t arg_num = VMState_getInputArgNum(vs);
    PikaObj* list = newNormalObj(constructor);
    __vm_List___init__(list);
    Stack stack = {0};
    stack_init(&stack);
    /* load to local stack to change sort */
    for (int i = 0; i < arg_num; i++) {
        Arg* arg = stack_popArg(&(vs->stack));
        stack_pushArg(&stack, arg);
    }
    for (int i = 0; i < arg_num; i++) {
        Arg* arg = stack_popArg(&stack);
        __vm_List_append(list, arg);
        arg_deinit(arg);
    }
    stack_deinit(&stack);
    return arg_newPtr(ARG_TYPE_OBJECT, list);
#else
    return VM_instruction_handler_NON(self, vs, "");
#endif
}

static Arg* VM_instruction_handler_LST(PikaObj* self, VMState* vs, char* data) {
    return _vm_create_list_or_tuple(self, vs, PIKA_TRUE);
}

void __vm_Dict___init__(PikaObj* self) {
    PikaDict* dict = New_dict();
    PikaDict* keys = New_dict();
    obj_setPtr(self, "dict", dict);
    obj_setPtr(self, "_keys", keys);
}

void __vm_Dict_set(PikaObj* self, Arg* arg, char* key) {
    PikaDict* dict = obj_getPtr(self, "dict");
    PikaDict* keys = obj_getPtr(self, "_keys");
    Arg* arg_key = arg_setStr(NULL, key, key);
    Arg* arg_new = arg_copy(arg);
    arg_setName(arg_new, key);
    dict_setArg(dict, arg_new);
    dict_setArg(keys, arg_key);
}

#if PIKA_BUILTIN_STRUCT_ENABLE
PikaObj* New_PikaStdData_Dict(Args* args);
#endif

static Arg* VM_instruction_handler_DCT(PikaObj* self, VMState* vs, char* data) {
#if PIKA_BUILTIN_STRUCT_ENABLE
    uint8_t arg_num = VMState_getInputArgNum(vs);
    PikaObj* dict = newNormalObj(New_PikaStdData_Dict);
    __vm_Dict___init__(dict);
    Stack stack = {0};
    stack_init(&stack);
    /* load to local stack to change sort */
    for (int i = 0; i < arg_num; i++) {
        Arg* arg = stack_popArg(&(vs->stack));
        stack_pushArg(&stack, arg);
    }
    for (int i = 0; i < arg_num / 2; i++) {
        Arg* key_arg = stack_popArg(&stack);
        Arg* val_arg = stack_popArg(&stack);
        __vm_Dict_set(dict, val_arg, arg_getStr(key_arg));
        arg_deinit(key_arg);
        arg_deinit(val_arg);
    }
    stack_deinit(&stack);
    return arg_newPtr(ARG_TYPE_OBJECT, dict);
#else
    return VM_instruction_handler_NON(self, vs, data);
#endif
}

static Arg* VM_instruction_handler_RUN(PikaObj* self, VMState* vs, char* data) {
    Args buffs = {0};
    Arg* return_arg = NULL;
    VMParameters* sub_locals = NULL;
    char* methodPath = data;
    PikaObj* method_host_obj;
    Arg* method_arg = NULL;
    Arg* host_arg = NULL;
    PIKA_BOOL isClass = PIKA_FALSE;
    char* sys_out;
    int arg_num_used = 0;
    TryInfo sub_try_info = {.try_state = TRY_STATE_NONE,
                            .try_result = TRY_RESULT_NONE};
    pika_assert(NULL != vs->try_info);
    if (vs->try_info->try_state == TRY_STATE_TOP ||
        vs->try_error_code == TRY_STATE_INNER) {
        sub_try_info.try_state = TRY_STATE_INNER;
    }
    if (strEqu(data, "")) {
        if (VMState_getInputArgNum(vs) < 2) {
            /* return arg directly */
            return_arg = stack_popArg(&(vs->stack));
            goto exit;
        }
        /* create a tuple */
        return_arg = _vm_create_list_or_tuple(self, vs, PIKA_FALSE);
        goto exit;
    }
    /* return tiny obj */
    if (strEqu(data, "TinyObj")) {
        return_arg = arg_newMetaObj(New_TinyObj);
        goto exit;
    }

    if (methodPath[0] == '.') {
        /* get method host obj from stack */
        Arg* stack_tmp[PIKA_ARG_NUM_MAX] = {0};
        int arg_num = VMState_getInputArgNum(vs);
        if (arg_num > PIKA_ARG_NUM_MAX) {
            __platform_printf(
                "[ERROR] Too many args in RUN instruction, please use bigger "
                "#define PIKA_ARG_NUM_MAX\n");
            while (1) {
            }
        }
        for (int i = 0; i < arg_num; i++) {
            stack_tmp[i] = stack_popArg(&(vs->stack));
        }
        host_arg = stack_tmp[arg_num - 1];
        if (argType_isObject(arg_getType(host_arg))) {
            method_host_obj = arg_getPtr(host_arg);
            arg_num_used++;
        }
        /* push back other args to stack */
        for (int i = arg_num - 2; i >= 0; i--) {
            stack_pushArg(&(vs->stack), stack_tmp[i]);
        }
    } else {
        /* get method host obj from self */
        method_host_obj = obj_getHostObjWithIsClass(self, methodPath, &isClass);
        /* get method host obj from local scope */
        if (NULL == method_host_obj) {
            method_host_obj =
                obj_getHostObjWithIsClass(vs->locals, methodPath, &isClass);
        }
    }

    if (NULL == method_host_obj) {
        /* error, not found object */
        VMState_setErrorCode(vs, PIKA_RES_ERR_ARG_NO_FOUND);
        __platform_printf("Error: method '%s' no found.\r\n", data);
        goto exit;
    }
    /* get method in local */
    method_arg = obj_getMethodArg(method_host_obj, methodPath);
    if (NULL == method_arg) {
        /* get method in global */
        method_arg = obj_getMethodArg(vs->globals, methodPath);
    }
    /* pika_assert method*/
    if (NULL == method_arg || ARG_TYPE_NONE == arg_getType(method_arg)) {
        /* error, method no found */
        VMState_setErrorCode(vs, PIKA_RES_ERR_ARG_NO_FOUND);
        __platform_printf("NameError: name '%s' is not defined\r\n", data);
        goto exit;
    }
    /* create sub local scope */
    sub_locals = New_PikaObj();
    /* load args from vmState to sub_local->list */
    arg_num_used += VMState_loadArgsFromMethodArg(
        vs, method_host_obj, sub_locals->list, method_arg, data, arg_num_used);

    /* load args faild */
    if (vs->error_code != 0) {
        goto exit;
    }

    /* run method arg */
    return_arg = __obj_runMethodArgWithState(method_host_obj, sub_locals,
                                             method_arg, &sub_try_info);

    /* __init__() */
    if (ARG_TYPE_OBJECT_NEW == arg_getType(return_arg)) {
        arg_setType(return_arg, ARG_TYPE_OBJECT);
        /* init object */
        PikaObj* new_obj = arg_getPtr(return_arg);
        Arg* method_arg = obj_getMethodArg(new_obj, "__init__");
        PikaObj* sub_locals = New_PikaObj();
        Arg* return_arg_init = NULL;
        if (NULL == method_arg) {
            goto init_exit;
        }
        VMState_loadArgsFromMethodArg(vs, new_obj, sub_locals->list, method_arg,
                                      "__init__", arg_num_used);
        /* load args faild */
        if (vs->error_code != 0) {
            goto init_exit;
        }
        return_arg_init = __obj_runMethodArgWithState(
            new_obj, sub_locals, method_arg, &sub_try_info);
    init_exit:
        if (NULL != return_arg_init) {
            arg_deinit(return_arg_init);
        }
        obj_deinit(sub_locals);
        arg_deinit(method_arg);
    }

    /* transfer sysOut */
    sys_out = obj_getSysOut(method_host_obj);
    if (NULL != sys_out) {
        args_setSysOut(vs->locals->list, sys_out);
    }
    /* transfer errCode */
    if (0 != obj_getErrorCode(method_host_obj)) {
        /* method error */
        VMState_setErrorCode(vs, PIKA_RES_ERR_RUNTIME_ERROR);
    }

    /* check try result */
    if (sub_try_info.try_result == TRY_RESULT_RAISE) {
        /* try error */
        VMState_setErrorCode(vs, PIKA_RES_ERR_RUNTIME_ERROR);
        vs->jmp = VM_JMP_RAISE;
    }

    goto exit;
exit:
    if (NULL != method_arg) {
        arg_deinit(method_arg);
    }
    if (NULL != sub_locals) {
        obj_deinit(sub_locals);
    }
    if (NULL != host_arg) {
        arg_deinit(host_arg);
    }
    if (isClass) {
        /* class method */
        obj_deinit(method_host_obj);
    }

    strsDeinit(&buffs);
    return return_arg;
}

static char* __get_transferd_str(Args* buffs, char* str, size_t* iout_p) {
    char* str_rep = strsReplace(buffs, str, "\\n", "\n");
    str_rep = strsReplace(buffs, str_rep, "\\r", "\r");
    str_rep = strsReplace(buffs, str_rep, "\\t", "\t");

    char* transfered_str = args_getBuff(buffs, strGetSize(str_rep));
    size_t i_out = 0;
    for (size_t i = 0; i < strGetSize(str_rep); i++) {
        /* eg. replace '\x33' to '3' */
        if ((str_rep[i] == '\\') && (str_rep[i + 1] == 'x')) {
            char hex_str[] = "0x00";
            hex_str[2] = str_rep[i + 2];
            hex_str[3] = str_rep[i + 3];
            char hex = (char)strtol(hex_str, NULL, 0);
            transfered_str[i_out++] = hex;
            i += 3;
            continue;
        }
        /* normal char */
        transfered_str[i_out++] = str_rep[i];
    }
    *iout_p = i_out;
    return transfered_str;
}

static Arg* VM_instruction_handler_STR(PikaObj* self, VMState* vs, char* data) {
    if (strIsContain(data, '\\')) {
        Args buffs = {0};
        size_t i_out = 0;
        char* transfered_str = __get_transferd_str(&buffs, data, &i_out);
        Arg* return_arg = New_arg(NULL);
        return_arg = arg_setStr(return_arg, "", transfered_str);
        strsDeinit(&buffs);
        return return_arg;
    }
    return arg_newStr(data);
}

static Arg* VM_instruction_handler_BYT(PikaObj* self, VMState* vs, char* data) {
    if (strIsContain(data, '\\')) {
        Args buffs = {0};
        size_t i_out = 0;
        char* transfered_str = __get_transferd_str(&buffs, data, &i_out);
        Arg* return_arg = New_arg(NULL);
        return_arg =
            arg_setBytes(return_arg, "", (uint8_t*)transfered_str, i_out);
        strsDeinit(&buffs);
        return return_arg;
    }
    return arg_newBytes((uint8_t*)data, strGetSize(data));
}

static Arg* VM_instruction_handler_OUT(PikaObj* self, VMState* vs, char* data) {
    Arg* outArg = stack_popArg(&(vs->stack));
    ArgType outArg_type = arg_getType(outArg);
    PikaObj* hostObj = vs->locals;
    /* match global_list */
    if (args_isArgExist(vs->locals->list, "__gl")) {
        char* global_list = args_getStr(vs->locals->list, "__gl");
        /* use a arg as buff */
        Arg* global_list_arg = arg_newStr(global_list);
        char* global_list_buff = arg_getStr(global_list_arg);
        /* for each arg arg in global_list */
        char token_buff[32] = {0};
        for (int i = 0; i < strCountSign(global_list, ',') + 1; i++) {
            char* global_arg = strPopToken(token_buff, global_list_buff, ',');
            /* matched global arg, hostObj set to global */
            if (strEqu(global_arg, data)) {
                hostObj = vs->globals;
            }
        }
        arg_deinit(global_list_arg);
    }
    /* use RunAs object */
    if (args_isArgExist(vs->locals->list, "__runAs")) {
        hostObj = args_getPtr(vs->locals->list, "__runAs");
    }
    /* set free object to nomal object */
    if (ARG_TYPE_OBJECT_NEW == outArg_type) {
        arg_setType(outArg, ARG_TYPE_OBJECT);
    }

    /* ouput arg to locals */
    obj_setArg_noCopy(hostObj, data, outArg);
    return NULL;
}

/* run as */
static Arg* VM_instruction_handler_RAS(PikaObj* self, VMState* vs, char* data) {
    if (strEqu(data, "$origin")) {
        /* use origin object to run */
        obj_removeArg(vs->locals, "__runAs");
        return NULL;
    }
    /* use "data" object to run */
    PikaObj* runAs = obj_getObj(vs->locals, data);
    args_setRef(vs->locals->list, "__runAs", runAs);
    return NULL;
}

static Arg* VM_instruction_handler_NUM(PikaObj* self, VMState* vs, char* data) {
    Arg* numArg = New_arg(NULL);
    /* hex */
    if (data[1] == 'x' || data[1] == 'X') {
        return arg_setInt(numArg, "", strtol(data, NULL, 0));
    }
    if (data[1] == 'o' || data[1] == 'O') {
        char strtol_buff[10] = {0};
        strtol_buff[0] = '0';
        __platform_memcpy(strtol_buff + 1, data + 2, strGetSize(data) - 2);
        return arg_setInt(numArg, "", strtol(strtol_buff, NULL, 0));
    }
    if (data[1] == 'b' || data[1] == 'B') {
        char strtol_buff[10] = {0};
        __platform_memcpy(strtol_buff, data + 2, strGetSize(data) - 2);
        return arg_setInt(numArg, "", strtol(strtol_buff, NULL, 2));
    }
    /* float */
    if (strIsContain(data, '.')) {
        return arg_setFloat(numArg, "", atof(data));
    }
    /* int */
    return arg_setInt(numArg, "", fast_atoi(data));
}

static Arg* VM_instruction_handler_JMP(PikaObj* self, VMState* vs, char* data) {
    vs->jmp = fast_atoi(data);
    return NULL;
}

static Arg* VM_instruction_handler_SER(PikaObj* self, VMState* vs, char* data) {
    vs->try_error_code = fast_atoi(data);
    return NULL;
}

static Arg* VM_instruction_handler_JEZ(PikaObj* self, VMState* vs, char* data) {
    int thisBlockDeepth;
    thisBlockDeepth = VMState_getBlockDeepthNow(vs);
    int jmp_expect = fast_atoi(data);
    Arg* pika_assertArg = stack_popArg(&(vs->stack));
    int pika_assert = arg_getInt(pika_assertArg);
    arg_deinit(pika_assertArg);
    char __else[] = "__else0";
    __else[6] = '0' + thisBlockDeepth;
    args_setInt(self->list, __else, !pika_assert);

    if (0 == pika_assert) {
        /* jump */
        vs->jmp = jmp_expect;
    }

    /* restore loop deepth */
    if (2 == jmp_expect && 0 == pika_assert) {
        int block_deepth_now = VMState_getBlockDeepthNow(vs);
        vs->loop_deepth = block_deepth_now;
    }

    return NULL;
}

static uint8_t VMState_getInputArgNum(VMState* vs) {
    InstructUnit* ins_unit_now = VMState_getInstructNow(vs);
    uint8_t invoke_deepth_this = instructUnit_getInvokeDeepth(ins_unit_now);
    int32_t pc_this = vs->pc;
    uint8_t num = 0;
    while (1) {
        ins_unit_now--;
        pc_this -= instructUnit_getSize(ins_unit_now);
        uint8_t invode_deepth = instructUnit_getInvokeDeepth(ins_unit_now);
        if (pc_this < 0) {
            break;
        }
        if (invode_deepth == invoke_deepth_this + 1) {
            num++;
        }
        if (instructUnit_getIsNewLine(ins_unit_now)) {
            break;
        }
        if (invode_deepth <= invoke_deepth_this) {
            break;
        }
    }
    return num;
}

static Arg* VM_instruction_handler_OPT(PikaObj* self, VMState* vs, char* data) {
    Arg* outArg = NULL;
    uint8_t input_arg_num = VMState_getInputArgNum(vs);
    Arg* arg2 = NULL;
    Arg* arg1 = NULL;
    if (input_arg_num == 2) {
        /* tow input */
        arg2 = stack_popArg(&(vs->stack));
        arg1 = stack_popArg(&(vs->stack));
    } else if (input_arg_num == 1) {
        /* only one input */
        arg2 = stack_popArg(&(vs->stack));
        arg1 = arg_newNull();
    }
    ArgType type_arg1 = arg_getType(arg1);
    ArgType type_arg2 = arg_getType(arg2);
    int num1_i = 0;
    int num2_i = 0;
    float num1_f = 0.0;
    float num2_f = 0.0;
    /* get int and float num */
    if (type_arg1 == ARG_TYPE_INT) {
        num1_i = arg_getInt(arg1);
        num1_f = (float)num1_i;
    } else if (type_arg1 == ARG_TYPE_FLOAT) {
        num1_f = arg_getFloat(arg1);
        num1_i = (int)num1_f;
    }
    if (type_arg2 == ARG_TYPE_INT) {
        num2_i = arg_getInt(arg2);
        num2_f = (float)num2_i;
    } else if (type_arg2 == ARG_TYPE_FLOAT) {
        num2_f = arg_getFloat(arg2);
        num2_i = (int)num2_f;
    }
    if (strEqu("+", data)) {
        if ((type_arg1 == ARG_TYPE_STRING) && (type_arg2 == ARG_TYPE_STRING)) {
            char* num1_s = NULL;
            char* num2_s = NULL;
            Args str_opt_buffs = {0};
            num1_s = arg_getStr(arg1);
            num2_s = arg_getStr(arg2);
            char* opt_str_out = strsAppend(&str_opt_buffs, num1_s, num2_s);
            outArg = arg_setStr(outArg, "", opt_str_out);
            strsDeinit(&str_opt_buffs);
            goto OPT_exit;
        }
        /* match float */
        if ((type_arg1 == ARG_TYPE_FLOAT) || type_arg2 == ARG_TYPE_FLOAT) {
            outArg = arg_setFloat(outArg, "", num1_f + num2_f);
            goto OPT_exit;
        }
        /* int is default */
        outArg = arg_setInt(outArg, "", num1_i + num2_i);
        goto OPT_exit;
    }
    if (strEqu("-", data)) {
        if (type_arg2 == ARG_TYPE_NONE) {
            if (type_arg1 == ARG_TYPE_INT) {
                outArg = arg_setInt(outArg, "", -num1_i);
                goto OPT_exit;
            }
            if (type_arg1 == ARG_TYPE_FLOAT) {
                outArg = arg_setFloat(outArg, "", -num1_f);
                goto OPT_exit;
            }
        }
        if ((type_arg1 == ARG_TYPE_FLOAT) || type_arg2 == ARG_TYPE_FLOAT) {
            outArg = arg_setFloat(outArg, "", num1_f - num2_f);
            goto OPT_exit;
        }
        outArg = arg_setInt(outArg, "", num1_i - num2_i);
        goto OPT_exit;
    }
    if (strEqu("*", data)) {
        if ((type_arg1 == ARG_TYPE_FLOAT) || type_arg2 == ARG_TYPE_FLOAT) {
            outArg = arg_setFloat(outArg, "", num1_f * num2_f);
            goto OPT_exit;
        }
        outArg = arg_setInt(outArg, "", num1_i * num2_i);
        goto OPT_exit;
    }
    if (strEqu("/", data)) {
        if (0 == num2_f) {
            VMState_setErrorCode(vs, PIKA_RES_ERR_OPERATION_FAILED);
            args_setSysOut(vs->locals->list,
                           "ZeroDivisionError: division by zero");
            outArg = NULL;
            goto OPT_exit;
        }
        outArg = arg_setFloat(outArg, "", num1_f / num2_f);
        goto OPT_exit;
    }
    if (strEqu("<", data)) {
        outArg = arg_setInt(outArg, "", num1_f < num2_f);
        goto OPT_exit;
    }
    if (strEqu(">", data)) {
        outArg = arg_setInt(outArg, "", num1_f > num2_f);
        goto OPT_exit;
    }
    if (strEqu("%", data)) {
        outArg = arg_setInt(outArg, "", num1_i % num2_i);
        goto OPT_exit;
    }
    if (strEqu("**", data)) {
        float res = 1;
        for (int i = 0; i < num2_i; i++) {
            res = res * num1_f;
        }
        outArg = arg_setFloat(outArg, "", res);
        goto OPT_exit;
    }
    if (strEqu("//", data)) {
        outArg = arg_setInt(outArg, "", num1_i / num2_i);
        goto OPT_exit;
    }
    if (strEqu("==", data) || strEqu("!=", data)) {
        int8_t is_equ = -1;
        if (type_arg1 == ARG_TYPE_NONE && type_arg2 == ARG_TYPE_NONE) {
            is_equ = 1;
            goto EQU_exit;
        }
        /* type not equl, and type is not int or float */
        if (type_arg1 != type_arg2) {
            if ((type_arg1 != ARG_TYPE_FLOAT) && (type_arg1 != ARG_TYPE_INT)) {
                is_equ = 0;
                goto EQU_exit;
            }
            if ((type_arg2 != ARG_TYPE_FLOAT) && (type_arg2 != ARG_TYPE_INT)) {
                is_equ = 0;
                goto EQU_exit;
            }
        }
        /* string compire */
        if (type_arg1 == ARG_TYPE_STRING) {
            is_equ = strEqu(arg_getStr(arg1), arg_getStr(arg2));
            goto EQU_exit;
        }
        /* bytes compire */
        if (type_arg1 == ARG_TYPE_BYTES) {
            if (arg_getBytesSize(arg1) != arg_getBytesSize(arg2)) {
                is_equ = 0;
                goto EQU_exit;
            }
            is_equ = 1;
            for (size_t i = 0; i < arg_getBytesSize(arg1); i++) {
                if (arg_getBytes(arg1)[i] != arg_getBytes(arg2)[i]) {
                    is_equ = 0;
                    goto EQU_exit;
                }
            }
            goto EQU_exit;
        }
        /* default: int and float */
        is_equ = ((num1_f - num2_f) * (num1_f - num2_f) < (float)0.000001);
        goto EQU_exit;
    EQU_exit:
        if (strEqu("==", data)) {
            outArg = arg_setInt(outArg, "", is_equ);
        } else {
            outArg = arg_setInt(outArg, "", !is_equ);
        }
        goto OPT_exit;
    }
    if (strEqu(">=", data)) {
        outArg = arg_setInt(
            outArg, "",
            (num1_f > num2_f) ||
                ((num1_f - num2_f) * (num1_f - num2_f) < (float)0.000001));
        goto OPT_exit;
    }
    if (strEqu("<=", data)) {
        outArg = arg_setInt(
            outArg, "",
            (num1_f < num2_f) ||
                ((num1_f - num2_f) * (num1_f - num2_f) < (float)0.000001));
        goto OPT_exit;
    }
    if (strEqu("&", data)) {
        outArg = arg_setInt(outArg, "", num1_i & num2_i);
        goto OPT_exit;
    }
    if (strEqu("|", data)) {
        outArg = arg_setInt(outArg, "", num1_i | num2_i);
        goto OPT_exit;
    }
    if (strEqu("~", data)) {
        outArg = arg_setInt(outArg, "", ~num2_i);
        goto OPT_exit;
    }
    if (strEqu(">>", data)) {
        outArg = arg_setInt(outArg, "", num1_i >> num2_i);
        goto OPT_exit;
    }
    if (strEqu("<<", data)) {
        outArg = arg_setInt(outArg, "", num1_i << num2_i);
        goto OPT_exit;
    }
    if (strEqu(" and ", data)) {
        outArg = arg_setInt(outArg, "", num1_i && num2_i);
        goto OPT_exit;
    }
    if (strEqu(" or ", data)) {
        outArg = arg_setInt(outArg, "", num1_i || num2_i);
        goto OPT_exit;
    }
    if (strEqu(" not ", data)) {
        outArg = arg_setInt(outArg, "", !num2_i);
        goto OPT_exit;
    }
    if (strEqu(" import ", data)) {
        outArg = NULL;
        goto OPT_exit;
    }
OPT_exit:
    arg_deinit(arg1);
    arg_deinit(arg2);
    if (NULL != outArg) {
        return outArg;
    }
    return NULL;
}

static Arg* __VM_instruction_handler_DEF(PikaObj* self,
                                         VMState* vs,
                                         char* data,
                                         uint8_t is_class) {
    int thisBlockDeepth = VMState_getBlockDeepthNow(vs);

    PikaObj* hostObj = vs->locals;
    uint8_t is_in_class = 0;
    /* use RunAs object */
    if (args_isArgExist(vs->locals->list, "__runAs")) {
        hostObj = args_getPtr(vs->locals->list, "__runAs");
        is_in_class = 1;
    }
    int offset = 0;
    /* byteCode */
    while (1) {
        InstructUnit* ins_unit_now = VMState_getInstructWithOffset(vs, offset);
        if (!instructUnit_getIsNewLine(ins_unit_now)) {
            offset += instructUnit_getSize();
            continue;
        }
        if (instructUnit_getBlockDeepth(ins_unit_now) == thisBlockDeepth + 1) {
            if (is_in_class) {
                class_defineObjectMethod(hostObj, data, (Method)ins_unit_now,
                                         self, vs->bytecode_frame);
            } else {
                if (is_class) {
                    class_defineRunTimeConstructor(hostObj, data,
                                                   (Method)ins_unit_now, self,
                                                   vs->bytecode_frame);
                } else {
                    class_defineStaticMethod(hostObj, data,
                                             (Method)ins_unit_now, self,
                                             vs->bytecode_frame);
                }
            }
            break;
        }
        offset += instructUnit_getSize();
    }

    return NULL;
}

static Arg* VM_instruction_handler_DEF(PikaObj* self, VMState* vs, char* data) {
    return __VM_instruction_handler_DEF(self, vs, data, 0);
}

static Arg* VM_instruction_handler_CLS(PikaObj* self, VMState* vs, char* data) {
    return __VM_instruction_handler_DEF(self, vs, data, 1);
}

static Arg* VM_instruction_handler_RET(PikaObj* self, VMState* vs, char* data) {
    /* exit jmp signal */
    vs->jmp = VM_JMP_EXIT;
    Arg* return_arg = stack_popArg(&(vs->stack));
    method_returnArg(vs->locals->list, return_arg);
    return NULL;
}

static Arg* VM_instruction_handler_RIS(PikaObj* self, VMState* vs, char* data) {
    Arg* err_arg = stack_popArg(&(vs->stack));
    PIKA_RES err = (PIKA_RES)arg_getInt(err_arg);
    VMState_setErrorCode(vs, err);
    arg_deinit(err_arg);
    /* raise jmp */
    if (vs->try_info->try_state == TRY_STATE_TOP) {
        vs->jmp = VM_JMP_RAISE;
    } else if (vs->try_info->try_state == TRY_STATE_INNER) {
        vs->try_info->try_result = TRY_RESULT_RAISE;
        return VM_instruction_handler_RET(self, vs, data);
    }
    return NULL;
}

static Arg* VM_instruction_handler_NEL(PikaObj* self, VMState* vs, char* data) {
    int thisBlockDeepth = VMState_getBlockDeepthNow(vs);
    char __else[] = "__else0";
    __else[6] = '0' + thisBlockDeepth;
    if (0 == args_getInt(self->list, __else)) {
        /* set __else flag */
        vs->jmp = fast_atoi(data);
    }
    return NULL;
}

static Arg* VM_instruction_handler_DEL(PikaObj* self, VMState* vs, char* data) {
    obj_removeArg(vs->locals, data);
    return NULL;
}

static Arg* VM_instruction_handler_EST(PikaObj* self, VMState* vs, char* data) {
    Arg* arg = obj_getArg(vs->locals, data);
    if (arg == NULL) {
        return arg_newInt(0);
    }
    if (ARG_TYPE_NONE == arg_getType(arg)) {
        return arg_newInt(0);
    }
    return arg_newInt(1);
}

static Arg* VM_instruction_handler_BRK(PikaObj* self, VMState* vs, char* data) {
    /* break jmp signal */
    vs->jmp = VM_JMP_BREAK;
    return NULL;
}

static Arg* VM_instruction_handler_CTN(PikaObj* self, VMState* vs, char* data) {
    /* continue jmp signal */
    vs->jmp = VM_JMP_CONTINUE;
    return NULL;
}

static Arg* VM_instruction_handler_GLB(PikaObj* self, VMState* vs, char* data) {
    Arg* global_list_buff = NULL;
    char* global_list = args_getStr(vs->locals->list, "__gl");
    /* create new global_list */
    if (NULL == global_list) {
        args_setStr(vs->locals->list, "__gl", data);
        goto exit;
    }
    /* append to exist global_list */
    global_list_buff = arg_newStr(global_list);
    global_list_buff = arg_strAppend(global_list_buff, ",");
    global_list_buff = arg_strAppend(global_list_buff, data);
    args_setStr(vs->locals->list, "__gl", arg_getStr(global_list_buff));
    goto exit;
exit:
    if (NULL != global_list_buff) {
        arg_deinit(global_list_buff);
    }
    return NULL;
}

static Arg* VM_instruction_handler_IMP(PikaObj* self, VMState* vs, char* data) {
    /* the module is already imported, skip. */
    if (obj_isArgExist(self, data)) {
        return NULL;
    }
    /* find cmodule in root object */
    extern PikaObj* __pikaMain;
    if (obj_isArgExist(__pikaMain, data)) {
        obj_setArg(self, data, obj_getArg(__pikaMain, data));
        return NULL;
    }
    /* import module from '__lib' */
    if (0 == obj_importModule(self, data)) {
        return NULL;
    }
    VMState_setErrorCode(vs, PIKA_RES_ERR_ARG_NO_FOUND);
    __platform_printf("ModuleNotFoundError: No module named '%s'\r\n", data);
    return NULL;
}

const VM_instruct_handler VM_instruct_handler_table[__INSTRCUTION_CNT] = {
#define __INS_TABLE
#include "__instruction_table.cfg"
};

enum Instruct pikaVM_getInstructFromAsm(char* ins_str) {
#define __INS_COMPIRE
#include "__instruction_table.cfg"
    return NON;
}

static int pikaVM_runInstructUnit(PikaObj* self,
                                  VMState* vs,
                                  InstructUnit* ins_unit) {
    enum Instruct instruct = instructUnit_getInstruct(ins_unit);
    Arg* return_arg;
    // char invode_deepth1_str[2] = {0};
    int32_t pc_next = vs->pc + instructUnit_getSize();
    char* data = VMState_getConstWithInstructUnit(vs, ins_unit);
    /* run instruct */
    pika_assert(NULL != vs->try_info);
    return_arg = VM_instruct_handler_table[instruct](self, vs, data);
    if (NULL != return_arg) {
        stack_pushArg(&(vs->stack), return_arg);
    }
    goto nextLine;
nextLine:
    /* exit */
    if (VM_JMP_EXIT == vs->jmp) {
        pc_next = VM_PC_EXIT;
        goto exit;
    }
    /* break */
    if (VM_JMP_BREAK == vs->jmp) {
        pc_next = vs->pc + VMState_getAddrOffsetOfBreak(vs);
        goto exit;
    }
    /* continue */
    if (VM_JMP_CONTINUE == vs->jmp) {
        pc_next = vs->pc + VMState_getAddrOffsetOfContinue(vs);
        goto exit;
    }
    /* raise */
    if (VM_JMP_RAISE == vs->jmp) {
        pc_next = vs->pc + VMState_getAddrOffsetOfRaise(vs);
        goto exit;
    }
    /* static jmp */
    if (vs->jmp != 0) {
        pc_next = vs->pc + VMState_getAddrOffsetFromJmp(vs);
        goto exit;
    }
    /* not jmp */
    pc_next = vs->pc + instructUnit_getSize();
    goto exit;
exit:
    vs->jmp = 0;
    /* reach the end */
    if (pc_next >= (int)VMState_getInstructArraySize(vs)) {
        return VM_PC_EXIT;
    }
    return pc_next;
}

VMParameters* pikaVM_runAsm(PikaObj* self, char* pikaAsm) {
    ByteCodeFrame bytecode_frame;
    byteCodeFrame_init(&bytecode_frame);
    byteCodeFrame_appendFromAsm(&bytecode_frame, pikaAsm);
    VMParameters* res = pikaVM_runByteCodeFrame(self, &bytecode_frame);
    byteCodeFrame_deinit(&bytecode_frame);
    return res;
}

static VMParameters* __pikaVM_runPyLines_or_byteCode(PikaObj* self,
                                                     char* py_lines,
                                                     uint8_t* bytecode) {
    uint8_t is_run_py;
    if (NULL != py_lines) {
        is_run_py = 1;
    } else if (NULL != bytecode) {
        is_run_py = 0;
    } else {
        return NULL;
    }
    Args buffs = {0};
    VMParameters* globals = NULL;
    ByteCodeFrame bytecode_frame_stack = {0};
    ByteCodeFrame* bytecode_frame_p = NULL;
    uint8_t is_use_heap_bytecode = 0;

    /*
     * the first obj_run, cache bytecode to heap, to support 'def' and 'class'
     */
    if (!args_isArgExist(self->list, "__first_bytecode")) {
        is_use_heap_bytecode = 1;
        /* load bytecode to heap */
        args_setHeapStruct(self->list, "__first_bytecode", bytecode_frame_stack,
                           byteCodeFrame_deinit);
        /* get bytecode_ptr from heap */
        bytecode_frame_p = args_getHeapStruct(self->list, "__first_bytecode");
    } else {
        /* not the first obj_run */
        /* save 'def' and 'class' to heap */
        if ((strIsStartWith(py_lines, "def ")) ||
            (strIsStartWith(py_lines, "class "))) {
            char* declear_name = strsGetFirstToken(&buffs, py_lines, ':');
            /* load bytecode to heap */
            args_setHeapStruct(self->list, declear_name, bytecode_frame_stack,
                               byteCodeFrame_deinit);
            /* get bytecode_ptr from heap */
            bytecode_frame_p = args_getHeapStruct(self->list, declear_name);
        } else {
            /* get bytecode_ptr from stack */
            bytecode_frame_p = &bytecode_frame_stack;
        }
    }

    /* load or generate byte code frame */
    if (is_run_py) {
        /* generate byte code */
        byteCodeFrame_init(bytecode_frame_p);
        if (1 == bytecodeFrame_fromMultiLine(bytecode_frame_p, py_lines)) {
            __platform_printf("[error]: Syntax error.\r\n");
            globals = NULL;
            goto exit;
        }
    } else {
        /* load bytecode */
        byteCodeFrame_loadByteCode(bytecode_frame_p, bytecode);
    }

    /* run byteCode */
    globals = pikaVM_runByteCodeFrame(self, bytecode_frame_p);
    goto exit;
exit:
    if (!is_use_heap_bytecode) {
        byteCodeFrame_deinit(&bytecode_frame_stack);
    }
    strsDeinit(&buffs);
    return globals;
}

VMParameters* pikaVM_runSingleFile(PikaObj* self, char* filename) {
    Args buffs = {0};
    Arg* file_arg = arg_loadFile(NULL, filename);
    pika_assert(NULL != file_arg);
    if (NULL == file_arg) {
        return NULL;
    }
    char* lines = (char*)arg_getBytes(file_arg);
    /* replace the "\r\n" to "\n" */
    lines = strsReplace(&buffs, lines, "\r\n", "\n");
    /* clear the void line */
    lines = strsReplace(&buffs, lines, "\n\n", "\n");
    /* add '\n' at the end */
    lines = strsAppend(&buffs, lines, "\n\n");
    VMParameters* res = pikaVM_run(self, lines);
    arg_deinit(file_arg);
    strsDeinit(&buffs);
    return res;
}

VMParameters* pikaVM_run(PikaObj* self, char* py_lines) {
    return __pikaVM_runPyLines_or_byteCode(self, py_lines, NULL);
}

VMParameters* pikaVM_runByteCode(PikaObj* self, uint8_t* bytecode) {
    return __pikaVM_runPyLines_or_byteCode(self, NULL, bytecode);
}

static void* constPool_getStart(ConstPool* self) {
    return self->content_start;
}

void constPool_update(ConstPool* self) {
    self->content_start = (void*)arg_getContent(self->arg_buff);
}

void constPool_init(ConstPool* self) {
    self->arg_buff = arg_newStr("");
    constPool_update(self);
    self->content_offset_now = 0;
    self->size = strGetSize(constPool_getStart(self)) + 1;
    self->output_redirect_fun = NULL;
    self->output_f = NULL;
}

void constPool_deinit(ConstPool* self) {
    if (NULL != self->arg_buff) {
        arg_deinit(self->arg_buff);
    }
}

void constPool_append(ConstPool* self, char* content) {
    uint16_t size = strGetSize(content) + 1;
    if (NULL == self->output_redirect_fun) {
        self->arg_buff = arg_append(self->arg_buff, content, size);
    } else {
        self->output_redirect_fun(self, content);
    };
    constPool_update(self);
    self->size += size;
}

char* constPool_getNow(ConstPool* self) {
    if (self->content_offset_now >= self->size) {
        /* is the end */
        return NULL;
    }
    return (char*)((uintptr_t)constPool_getStart(self) +
                   (uintptr_t)(self->content_offset_now));
}

uint16_t constPool_getLastOffset(ConstPool* self) {
    return self->size;
}

uint16_t constPool_getOffsetByData(ConstPool* self, char* data) {
    uint16_t ptr_befor = self->content_offset_now;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    uint16_t offset_out = 65535;
    while (1) {
        if (NULL == constPool_getNext(self)) {
            goto exit;
        }
        if (strEqu(data, constPool_getNow(self))) {
            offset_out = self->content_offset_now;
            goto exit;
        }
    }
exit:
    /* retore ptr_now */
    self->content_offset_now = ptr_befor;
    return offset_out;
}

char* constPool_getNext(ConstPool* self) {
    self->content_offset_now += strGetSize(constPool_getNow(self)) + 1;
    return constPool_getNow(self);
}

char* constPool_getByIndex(ConstPool* self, uint16_t index) {
    uint16_t ptr_befor = self->content_offset_now;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    for (uint16_t i = 0; i < index; i++) {
        constPool_getNext(self);
    }
    char* res = constPool_getNow(self);
    /* retore ptr_now */
    self->content_offset_now = ptr_befor;
    return res;
}

void constPool_print(ConstPool* self) {
    uint16_t ptr_befor = self->content_offset_now;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    while (1) {
        if (NULL == constPool_getNext(self)) {
            goto exit;
        }
        uint16_t offset = self->content_offset_now;
        __platform_printf("%d: %s\r\n", offset, constPool_getNow(self));
    }
exit:
    /* retore ptr_now */
    self->content_offset_now = ptr_befor;
    return;
}

void byteCodeFrame_init(ByteCodeFrame* self) {
    /* init to support append,
       if only load static bytecode,
       can not init */
    constPool_init(&(self->const_pool));
    instructArray_init(&(self->instruct_array));
}

void byteCodeFrame_loadByteCode(ByteCodeFrame* self, uint8_t* bytes) {
    uint16_t* ins_size_p = (uint16_t*)bytes;
    void* ins_start_p = (uint16_t*)(bytes + 2);
    uint16_t* const_size_p =
        (uint16_t*)((uintptr_t)ins_start_p + (uintptr_t)(*ins_size_p));
    self->instruct_array.size = *ins_size_p;
    self->instruct_array.content_start = ins_start_p;
    self->const_pool.size = *const_size_p;
    self->const_pool.content_start = (char*)((uintptr_t)const_size_p + 2);
}

void byteCodeFrame_deinit(ByteCodeFrame* self) {
    constPool_deinit(&(self->const_pool));
    instructArray_deinit(&(self->instruct_array));
}

void instructArray_init(InstructArray* self) {
    self->arg_buff = arg_newNull();
    instructArray_update(self);
    self->size = 0;
    self->content_offset_now = 0;
    self->output_redirect_fun = NULL;
    self->output_f = NULL;
}

void instructArray_deinit(InstructArray* self) {
    if (NULL != self->arg_buff) {
        arg_deinit(self->arg_buff);
    }
}

void instructArray_append(InstructArray* self, InstructUnit* ins_unit) {
    if (NULL == self->output_redirect_fun) {
        self->arg_buff =
            arg_append(self->arg_buff, ins_unit, instructUnit_getSize());
    } else {
        self->output_redirect_fun(self, ins_unit);
    };
    instructArray_update(self);
    self->size += instructUnit_getSize();
}

void instructUnit_init(InstructUnit* ins_unit) {
    ins_unit->deepth = 0;
    ins_unit->const_pool_index = 0;
    ins_unit->isNewLine_instruct = 0;
}

static void* instructArray_getStart(InstructArray* self) {
    return self->content_start;
}

void instructArray_update(InstructArray* self) {
    self->content_start = (void*)arg_getContent(self->arg_buff);
}

InstructUnit* instructArray_getNow(InstructArray* self) {
    if (self->content_offset_now >= self->size) {
        /* is the end */
        return NULL;
    }
    return (InstructUnit*)((uintptr_t)instructArray_getStart(self) +
                           (uintptr_t)(self->content_offset_now));
}

InstructUnit* instructArray_getNext(InstructArray* self) {
    self->content_offset_now += instructUnit_getSize();
    return instructArray_getNow(self);
}

static char* instructUnit_getInstructStr(InstructUnit* self) {
#define __INS_GET_INS_STR
#include "__instruction_table.cfg"
    return "NON";
}

void instructUnit_print(InstructUnit* self) {
    if (instructUnit_getIsNewLine(self)) {
        __platform_printf("B%d\r\n", instructUnit_getBlockDeepth(self));
    }
    __platform_printf("%d %s #%d\r\n", instructUnit_getInvokeDeepth(self),
                      instructUnit_getInstructStr(self),
                      self->const_pool_index);
}

static void instructUnit_printWithConst(InstructUnit* self,
                                        ConstPool* const_pool) {
    // if (instructUnit_getIsNewLine(self)) {
    //     __platform_printf("B%d\r\n", instructUnit_getBlockDeepth(self));
    // }
    __platform_printf("%s %s \t\t(#%d)\r\n", instructUnit_getInstructStr(self),
                      constPool_getByOffset(const_pool, self->const_pool_index),
                      self->const_pool_index);
}

void instructArray_printWithConst(InstructArray* self, ConstPool* const_pool) {
    uint16_t offset_befor = self->content_offset_now;
    self->content_offset_now = 0;
    while (1) {
        InstructUnit* ins_unit = instructArray_getNow(self);
        if (NULL == ins_unit) {
            goto exit;
        }
        instructUnit_printWithConst(ins_unit, const_pool);
        instructArray_getNext(self);
    }
exit:
    self->content_offset_now = offset_befor;
    return;
}

void instructArray_print(InstructArray* self) {
    uint16_t offset_befor = self->content_offset_now;
    self->content_offset_now = 0;
    while (1) {
        InstructUnit* ins_unit = instructArray_getNow(self);
        if (NULL == ins_unit) {
            goto exit;
        }
        instructUnit_print(ins_unit);
        instructArray_getNext(self);
    }
exit:
    self->content_offset_now = offset_befor;
    return;
}

void instructArray_printAsArray(InstructArray* self) {
    uint16_t offset_befor = self->content_offset_now;
    self->content_offset_now = 0;
    uint8_t line_num = 12;
    uint16_t g_i = 0;
    uint8_t* ins_size_p = (uint8_t*)&self->size;
    __platform_printf("0x%02x, ", *(ins_size_p));
    __platform_printf("0x%02x, ", *(ins_size_p + (uintptr_t)1));
    __platform_printf("/* instruct array size */\n");
    while (1) {
        InstructUnit* ins_unit = instructArray_getNow(self);
        if (NULL == ins_unit) {
            goto exit;
        }
        for (int i = 0; i < (int)instructUnit_getSize(ins_unit); i++) {
            g_i++;
            __platform_printf("0x%02x, ", *((uint8_t*)ins_unit + (uintptr_t)i));
            if (g_i % line_num == 0) {
                __platform_printf("\n");
            }
        }
        instructArray_getNext(self);
    }
exit:
    __platform_printf("/* instruct array */\n");
    self->content_offset_now = offset_befor;
    return;
}

size_t byteCodeFrame_getSize(ByteCodeFrame* bf) {
    return bf->const_pool.size + bf->instruct_array.size;
}

void byteCodeFrame_print(ByteCodeFrame* self) {
    constPool_print(&(self->const_pool));
    instructArray_printWithConst(&(self->instruct_array), &(self->const_pool));
    __platform_printf("---------------\r\n");
    __platform_printf("byte code size: %d\r\n",
                      self->const_pool.size + self->instruct_array.size);
}

void VMState_solveUnusedStack(VMState* vs) {
    uint8_t top = stack_getTop(&(vs->stack));
    for (int i = 0; i < top; i++) {
        Arg* arg = stack_popArg(&(vs->stack));
        ArgType type = arg_getType(arg);
        if (type == ARG_TYPE_NONE) {
            arg_deinit(arg);
            continue;
        }
        if (vs->line_error_code != 0) {
            arg_deinit(arg);
            continue;
        }
        if (argType_isObject(type)) {
            char* res = obj_toStr(arg_getPtr(arg));
            __platform_printf("%s\r\n", res);
        } else if (type == ARG_TYPE_INT) {
            __platform_printf("%d\r\n", (int)arg_getInt(arg));
        } else if (type == ARG_TYPE_FLOAT) {
            __platform_printf("%f\r\n", arg_getFloat(arg));
        } else if (type == ARG_TYPE_STRING) {
            __platform_printf("%s\r\n", arg_getStr(arg));
        } else if (type == ARG_TYPE_BYTES) {
            arg_printBytes(arg);
        } else if (ARG_TYPE_POINTER == type ||
                   ARG_TYPE_METHOD_NATIVE_CONSTRUCTOR) {
            __platform_printf("%p\r\n", arg_getPtr(arg));
        }
        arg_deinit(arg);
    }
}

static VMParameters* __pikaVM_runByteCodeFrameWithState(
    PikaObj* self,
    VMParameters* locals,
    VMParameters* globals,
    ByteCodeFrame* bytecode_frame,
    uint16_t pc,
    TryInfo* try_info) {
    pika_assert(NULL != try_info);
    int size = bytecode_frame->instruct_array.size;
    /* locals is the local scope */
    VMState vs = {
        .bytecode_frame = bytecode_frame,
        .locals = locals,
        .globals = globals,
        .jmp = 0,
        .pc = pc,
        .loop_deepth = 0,
        .error_code = PIKA_RES_OK,
        .line_error_code = PIKA_RES_OK,
        .try_error_code = PIKA_RES_OK,
        .try_info = try_info,
    };
    stack_init(&(vs.stack));
    while (vs.pc < size) {
        if (vs.pc == VM_PC_EXIT) {
            break;
        }
        InstructUnit* this_ins_unit = VMState_getInstructNow(&vs);
        if (instructUnit_getIsNewLine(this_ins_unit)) {
            VMState_solveUnusedStack(&vs);
            stack_reset(&(vs.stack));
            vs.error_code = 0;
            vs.line_error_code = 0;
        }
        vs.pc = pikaVM_runInstructUnit(self, &vs, this_ins_unit);
        if (0 != vs.error_code) {
            vs.line_error_code = vs.error_code;
            InstructUnit* head_ins_unit = this_ins_unit;
            /* get first ins of a line */
            while (1) {
                if (instructUnit_getIsNewLine(head_ins_unit)) {
                    break;
                }
                head_ins_unit--;
            }
            if (vs.try_info->try_state) {
                vs.try_error_code = vs.error_code;
            }
            /* print inses of a line */
            if (!vs.try_info->try_state) {
                while (1) {
                    if (head_ins_unit != this_ins_unit) {
                        __platform_printf("   ");
                    } else {
                        __platform_printf(" -> ");
                    }
                    instructUnit_printWithConst(head_ins_unit,
                                                &(bytecode_frame->const_pool));
                    head_ins_unit++;
                    if (head_ins_unit > this_ins_unit) {
                        break;
                    }
                }
            }
            __platform_error_handle();
            vs.error_code = 0;
        }
    }
    VMState_solveUnusedStack(&vs);
    stack_deinit(&(vs.stack));
    return locals;
}

VMParameters* pikaVM_runByteCodeFrame(PikaObj* self,
                                      ByteCodeFrame* byteCode_frame) {
    TryInfo try_info = {.try_state = TRY_STATE_NONE,
                        .try_result = TRY_RESULT_NONE};
    try_info.try_state = TRY_STATE_NONE;
    return __pikaVM_runByteCodeFrameWithState(self, self, self, byteCode_frame,
                                              0, &try_info);
}

char* constPool_getByOffset(ConstPool* self, uint16_t offset) {
    return (char*)((uintptr_t)constPool_getStart(self) + (uintptr_t)offset);
}

InstructUnit* instructArray_getByOffset(InstructArray* self, int32_t offset) {
    return (InstructUnit*)((uintptr_t)instructArray_getStart(self) +
                           (uintptr_t)offset);
}

void constPool_printAsArray(ConstPool* self) {
    uint8_t* const_size_str = (uint8_t*)&(self->size);
    __platform_printf("0x%02x, ", *(const_size_str));
    __platform_printf("0x%02x, ", *(const_size_str + (uintptr_t)1));
    __platform_printf("/* const pool size */\n");
    uint16_t ptr_befor = self->content_offset_now;
    uint8_t line_num = 12;
    uint16_t g_i = 0;
    /* set ptr_now to begin */
    self->content_offset_now = 0;
    __platform_printf("0x00, ");
    while (1) {
        if (NULL == constPool_getNext(self)) {
            goto exit;
        }
        char* data_each = constPool_getNow(self);
        /* todo start */
        for (uint32_t i = 0; i < strGetSize(data_each) + 1; i++) {
            __platform_printf("0x%02x, ", *(data_each + (uintptr_t)i));
            g_i++;
            if (g_i % line_num == 0) {
                __platform_printf("\n");
            }
        }
        /* todo end */
    }
exit:
    /* retore ptr_now */
    __platform_printf("/* const pool */\n");
    self->content_offset_now = ptr_befor;
    return;
}

void byteCodeFrame_printAsArray(ByteCodeFrame* self) {
    __platform_printf("const uint8_t bytes[] = {\n");
    instructArray_printAsArray(&(self->instruct_array));
    constPool_printAsArray(&(self->const_pool));
    __platform_printf("};\n");
    __platform_printf("pikaVM_runByteCode(self, (uint8_t*)bytes);\n");
}

PikaObj* pikaVM_runFile(PikaObj* self, char* file_name) {
    Args buffs = {0};
    char* module_name = strsCopy(&buffs, file_name);
    strPopLastToken(module_name, '.');

    __platform_printf("(pikascript) pika compiler:\r\n");
    PikaMaker* maker = New_PikaMaker();
    pikaMaker_compileModuleWithDepends(maker, module_name);
    pikaMaker_linkCompiledModules(maker, "pikaModules_cache.py.a");
    obj_deinit(maker);
    __platform_printf("(pikascript) all succeed.\r\n\r\n");

    pikaMemMaxReset();
    Obj_linkLibraryFile(self, "pikascript-api/pikaModules_cache.py.a");
    self = pikaVM_runSingleFile(self, file_name);
    strsDeinit(&buffs);
    return self;
}
