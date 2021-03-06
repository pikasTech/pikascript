#include "test_common.h"

#if PIKA_SYNTAX_SLICE_ENABLE
TEST(stddata, test1) {
    /* init */
    pikaMemInfo.heapUsedMax = 0;
    PikaObj* pikaMain = newRootObj("pikaMain", New_PikaMain);
    /* run */
    __platform_printf("BEGIN\r\n");
    pikaVM_runSingleFile(pikaMain, "../../examples/BuiltIn/dict.py");
    /* collect */
    /* assert */
    EXPECT_STREQ(log_buff[0], "{'len': 3, 'list': [1, 2, 3]}\r\n");
    EXPECT_STREQ(log_buff[1], "dict_keys([c, b, a])\r\n");
    EXPECT_STREQ(log_buff[2], "a\r\n");
    EXPECT_STREQ(log_buff[3], "b\r\n");
    EXPECT_STREQ(log_buff[4], "c\r\n");
    EXPECT_STREQ(log_buff[5], "1\r\n");
    EXPECT_STREQ(log_buff[6], "2\r\n");
    EXPECT_STREQ(log_buff[7], "test\r\n");
    EXPECT_STREQ(log_buff[8], "{'c': test, 'b': 2, 'a': 1}\r\n");
    EXPECT_STREQ(log_buff[9], "BEGIN\r\n");
    /* deinit */
    obj_deinit(pikaMain);
    EXPECT_EQ(pikaMemNow(), 0);
}
#endif

/* test b2a_hex */
TEST(stddata, test2) {
    /* init */
    pikaMemInfo.heapUsedMax = 0;
    PikaObj* pikaMain = newRootObj("pikaMain", New_PikaMain);
    /* run */
    __platform_printf("BEGIN\r\n");
    obj_run(pikaMain,
            "import binascii\n"
            "res = str(binascii.b2a_hex(b'德卡科技'))\n");
    /* collect */
    char* res = obj_getStr(pikaMain, "res");
    /* assert */
    EXPECT_STREQ(res, "E5BEB7E58DA1E7A791E68A80");
    /* deinit */
    obj_deinit(pikaMain);
}

/* test a2b_hex */
TEST(stddata, a2b_hex) {
    /* init */
    pikaMemInfo.heapUsedMax = 0;
    PikaObj* pikaMain = newRootObj("pikaMain", New_PikaMain);
    /* run */
    __platform_printf("BEGIN\r\n");
    obj_run(pikaMain,
            "import binascii\n"
            "text = binascii.a2b_hex('e4b8ade69687e6b58be8af95e794a8e4be8b')\n"
            "res = str(text)\n");
    /* collect */
    char* res = obj_getStr(pikaMain, "res");
    /* assert */
    EXPECT_STREQ(res, "中文测试用例");
    /* deinit */
    obj_deinit(pikaMain);
}

#if PIKA_SYNTAX_IMPORT_EX_ENABLE
TEST(stddata, encode_decode) {
    /* init */
    pikaMemInfo.heapUsedMax = 0;
    PikaObj* pikaMain = newRootObj("pikaMain", New_PikaMain);
    /* run */
    __platform_printf("BEGIN\r\n");
    pikaVM_runSingleFile(pikaMain, "../../examples/BuiltIn/encode_decode.py");
    /* collect */
    char* b_s = obj_getStr(pikaMain, "b_s");
    uint8_t* s_b = obj_getBytes(pikaMain, "s_b");
    /* assert */
    EXPECT_STREQ(b_s, "test");
    EXPECT_EQ(s_b[0], 't');
    EXPECT_EQ(s_b[1], 'e');
    EXPECT_EQ(s_b[2], 's');
    EXPECT_EQ(s_b[3], 't');
    /* deinit */
    obj_deinit(pikaMain);
}
#endif

#if PIKA_FILEIO_ENABLE
TEST(stddata, fileio) {
    /* init */
    pikaMemInfo.heapUsedMax = 0;
    PikaObj* pikaMain = newRootObj("pikaMain", New_PikaMain);
    /* run */
    __platform_printf("BEGIN\r\n");
    pikaVM_runSingleFile(pikaMain, "../../examples/BuiltIn/file.py");
    Arg* s = obj_getArg(pikaMain, "s");
    Arg* b = obj_getArg(pikaMain, "b");
    /* assert */
    EXPECT_EQ(arg_getType(s), ARG_TYPE_STRING);
    EXPECT_EQ(arg_getType(b), ARG_TYPE_BYTES);
    /* deinit */
    obj_deinit(pikaMain);
}
#endif
