#include "PikaPlatform.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

PIKA_WEAK void __platformDisableIrqHandle() {
    /* disable irq to support thread */
}

PIKA_WEAK void __platformEnableIrqHandle() {
    /* disable irq to support thread */
}

PIKA_WEAK void* __platformMalloc(size_t size) {
    return malloc(size);
}

PIKA_WEAK void __platformFree(void* ptr) {
    return free(ptr);
}

PIKA_WEAK void __platformPrintf(char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char sysOut[128] = {0};
    vprintf(fmt, args);
    va_end(args);
}

PIKA_WEAK char* __platformLoadPikaAsm(){
    /* faild */
    return NULL;
}

PIKA_WEAK int32_t __platformSavePikaAsm(char *PikaAsm){
    /* faild */
    return 1;
}

PIKA_WEAK uint8_t __platformAsmIsToFlash(char *pyMultiLine){
    /* not to flash */
    return 0;
}

PIKA_WEAK int32_t __platformSavePikaAsmEOF(){
    return 1;
}
