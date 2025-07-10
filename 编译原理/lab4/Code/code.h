#ifndef CODE_H
#define CODE_H

#include "ir.h"

typedef struct Register_ Register_t;
typedef Register_t* Register;
typedef struct Variable_ Variable_t;
typedef Variable_t* Variable;
typedef struct Frame_ Frame_t;
typedef Frame_t* Frame;

struct Register_{
    int no;             // 寄存器的编号
    bool isInUse;         // 是否被占用
    int lastUse;       // 距离上次使用的间隔
    Variable var;       // 寄存器储存的变量
};

struct Variable_{
    Operand op;         // 代表的操作数
    int offset;         // 距离所在栈帧的 fp 指针的偏移量
    Variable next;
};

struct Frame_{
    char name[32];     // 相当于函数的名称
    Variable varlist;   // 栈帧的变量列表
    Frame next;
};

void updateFramePointer(char* name);
void initRegisters();
void freeRegisters();
void clearRegisters();
Variable insertVariable(Operand op, Frame frame);
Variable lookupVariable(Operand op, Frame frame);
Variable handleVariable(Operand op, Frame frame);
void initFrames(InterCode ir);
bool opEqual(Operand op1, Operand op2);
int handleOp(FILE* fp, Operand op, bool load);
int getRegForVar(FILE* fp, Variable var);
void pushRegsToStack(FILE* fp);
void popStackToRegs(FILE* fp);
void updateRound(int no);
void initObjectCode(FILE* fp);
void printObjectCode(char* filepath, InterCode ir);

#endif