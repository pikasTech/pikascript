#ifndef __PIKA__PARSER__H
#define __PIKA__PARSER__H
#include "dataQueueObj.h"

typedef QueueObj AST;
AST* pikaParse(char* line);
int32_t AST_deinit(AST* ast);
char* AST_toShell(AST* ast, Args* buffs);

#endif