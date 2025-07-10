#include "code.h"
#include "assert.h"

const char *REG_NAME[32] = {
    "$0", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};
    
const char *reserved[] = {
    "add", "sub", "mul", "div", "rem", "neg",
    "and", "or", "xor", "nor", "not"
    // "sll", "srl", "sra", "slt", "sltu",
    // "seq", "sne", "li", "la", "move",
    // "mfhi", "mflo", "lw", "sw", "lb", "sb",
    // "j", "jr", "jal", "beq", "bne", "ble", "blt", "bgt", "bge", "slti",
    // "syscall", "break",
    // ".data", ".text", ".globl", ".asciiz", ".word"
};

#define REG_START 8
#define REG_END 26
#define RESERVED_COUNT (sizeof(reserved) / sizeof(reserved[0]))

Register regs[32];
Frame frameStack = NULL;
Frame currentFrame = NULL;

int is_reserved(const char *name)
{
    for (int i = 0; i < RESERVED_COUNT; i++)
    {
        if (strcmp(name, reserved[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

char* changename(const char *name)
{
    size_t len = strlen(name);
    char *new_name;

    if (is_reserved(name))
    {
        new_name = (char *)malloc(len + 2);
        if (!new_name)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        strcpy(new_name, name);
        new_name[len] = '_';
        new_name[len + 1] = '\0';
    }
    else
    {
        new_name = (char *)malloc(len + 1);
        if (!new_name)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        strcpy(new_name, name);
    }

    return new_name;
}

void updateFramePointer(char *name)
{
    Frame ptr = frameStack;
    while (ptr != NULL)
    {
        if (strcmp(ptr->name, name) == 0)
        {
            currentFrame = ptr;
            return;
        }
        ptr = ptr->next;
    }
    currentFrame = NULL;
}

void initRegisters()
{
    for (int i = 0; i < 32; i++)
    {
        regs[i] = (Register)malloc(sizeof(Register_t));
        assert(regs[i] != NULL);
        regs[i]->no = i;
        regs[i]->isInUse = false;
        regs[i]->var = NULL;
        regs[i]->lastUse = 0;
    }
}

void freeRegisters() 
{
    for (int i = 8; i <= 25; i++) {
        regs[i]->isInUse = false;
    }
}

void clearRegisters()
{
    for (int i = 8; i <= 25; i++)
    {
        regs[i]->isInUse = false;
        regs[i]->var = NULL;
        regs[i]->lastUse = 0;
    }
}

Variable lookupVariable(Operand op, Frame frame) {
    if (frame == NULL || op == NULL) {
        return NULL;
    }

    Variable ptr = frame->varlist;
    while (ptr != NULL) {
        if (opEqual(ptr->op, op)) {
            return ptr;
        }
        ptr = ptr->next;
    }

    return NULL;
}

Variable insertVariable(Operand op, Frame frame) {
    if (frame == NULL || op == NULL) {
        return NULL;
    }

    Variable var = (Variable)malloc(sizeof(Variable_t));
    var->op = op;

    if (frame->varlist == NULL) {
        var->offset = getSizeof(var->op->type);
    } else {
        var->offset = getSizeof(var->op->type) + frame->varlist->offset;
    }
    var->next = frame->varlist;
    frame->varlist = var;
    return var;
}

Variable handleVariable(Operand op, Frame frame) {
    Variable v = lookupVariable(op, frame);
    if (v != NULL) {
        return v;
    }
    return insertVariable(op, frame);
}

void tryHandleVariable(Operand op, Frame frameStack) {
    if (op && (op->kind == VARIABLE || op->kind == TEMP_VAR)) {
        handleVariable(op, frameStack);
    }
}

// 首先遍历一遍IR,确定每个变量的偏移量
void initFrames(InterCode ir) {
    InterCode code = ir; 
    while (code != NULL) {
        switch (code->kind) {
            case RETURN_: case DEC_: case ARG_: case CALL_: case PARAM_: case READ_: case WRITE_:
                tryHandleVariable(code->ops[0], frameStack);
                break;
            case ASSIGN_: case MEM_: case IF_GOTO_:
                tryHandleVariable(code->ops[0], frameStack);
                tryHandleVariable(code->ops[1], frameStack);
                break;
            case PLUS_: case SUB_: case MULTI_: case DIV_:
                tryHandleVariable(code->ops[0], frameStack);
                tryHandleVariable(code->ops[1], frameStack);
                tryHandleVariable(code->ops[2], frameStack);
                break;
            case FUNC_: {
                Frame frame = (Frame)malloc(sizeof(Frame_t));
                strcpy(frame->name, code->ops[0]->name);
                frame->varlist = NULL;
                frame->next = frameStack;
                frameStack = frame;
                break;
            }
            default:
                break;
        }        
        code = code->next;
    }
}

bool opEqual(Operand op1, Operand op2) {
    if (op1 == NULL && op2 == NULL) return true;
    if (op1 == NULL || op2 == NULL) return false;
    if (op1->kind != op2->kind) return false;
    switch (op1->kind) {
        case VARIABLE: case FUNC:
            return strcmp(op1->name, op2->name) == 0;
        case TEMP_VAR: case LABEL:
            return op1->no == op2->no;
        case CONSTANT:
            return op1->value == op2->value;
        case GET_ADDR: case GET_VAL:
            return opEqual(op1->opr, op2->opr);
        default:
            return false;
    }
}

int handleOp(FILE *fp, Operand op, bool load) {
    assert(op != NULL);
    switch (op->kind) {
        case CONSTANT:
            if (op->value == 0) {
                return 0; // $zero register
            } else {
                Variable var = handleVariable(op, currentFrame);
                int regno = getRegForVar(fp, var);
                if (load) {
                    fprintf(fp, "  li %s, %d\n", REG_NAME[regno], op->value);
                }
                return regno;
            }
        case VARIABLE:
        case TEMP_VAR: {
            Variable var = handleVariable(op, currentFrame);
            int regno = getRegForVar(fp, var);
            if (load) {
                fprintf(fp, "  lw %s, %d($fp)\n", REG_NAME[regno], -var->offset);
            }
            return regno;
        }
        case GET_VAL: {
            int regno = handleOp(fp, op->opr, load);
            fprintf(fp, "  lw %s, 0(%s)\n", REG_NAME[regno], REG_NAME[regno]);
            return regno;
        }
        case GET_ADDR: {
            int regno = handleOp(fp, op->opr, load);
            Variable var = handleVariable(op->opr, currentFrame);
            fprintf(fp, "  addi %s, $fp, %d\n", REG_NAME[regno], -var->offset);
            return regno;
        }
        default:
            fprintf(stderr, "Unsupported operand kind %d\n", op->kind);
            assert(0 && "Unknown operand kind");
            return -1;
    }
}

int getRegForVar(FILE *fp, Variable var) {
    assert(var != NULL);
    // 优先分配空闲寄存器
    for (int i = REG_START; i < REG_END; i++) {
        if (!regs[i]->isInUse) {
            regs[i]->isInUse = true;
            regs[i]->var = var;
            updateRound(i);
            return i;
        }
        // 如果这个寄存器正好已经用于当前变量，直接复用
        if (regs[i]->var == var) {
            updateRound(i);
            return i;
        }
    }
    // 所有寄存器都在用，采用LRU替换策略
    int replaceIdx = -1;
    int maxRound = -1;
    for (int i = REG_START; i < REG_END; i++) {
        if (regs[i]->lastUse > maxRound) {
            maxRound = regs[i]->lastUse;
            replaceIdx = i;
        }
    }
    assert(replaceIdx != -1);
    assert(regs[replaceIdx]->var != NULL);
    // 回写旧变量
    fprintf(fp, "  sw %s, %d($fp)\n",
            REG_NAME[replaceIdx], -regs[replaceIdx]->var->offset);
    // 替换为新变量
    regs[replaceIdx]->isInUse = true;
    regs[replaceIdx]->var = var;
    updateRound(replaceIdx);
    return replaceIdx;
}

void updateRound(int no) {
    for (int i = REG_START; i < REG_END; i++) {
        regs[i]->lastUse++;
    }
    regs[no]->lastUse = 0;
}

void pushRegsToStack(FILE *fp) {
    fprintf(fp, "  addi $sp, $sp, -72\n");
    for (int i = REG_END; i >= REG_START; i--) {
        fprintf(fp, "  sw %s, %d($sp)\n", REG_NAME[i], 4 * (i - 8));
    }
    fprintf(fp, "\n");
}

void popStackToRegs(FILE *fp) {
    fprintf(fp, "\n");
    for (int i = REG_START; i <= REG_END; i++) {
        fprintf(fp, "  lw %s, %d($sp)\n", REG_NAME[i], 4 * (i - 8));
    }
    fprintf(fp, "  addi $sp, $sp, 72\n\n");
}

// 初始化目标代码文件
void initObjectCode(FILE *fp) {
    const char *lines[] = {
        ".data",
        "_prompt: .asciiz \"Enter an integer:\"",
        "_ret: .asciiz \"\\n\"",
        ".globl main",
        ".text",
        "read:",
        "  li $v0, 4",
        "  la $a0, _prompt",
        "  syscall",
        "  li $v0, 5",
        "  syscall",
        "  jr $ra",
        "",
        "write:",
        "  li $v0, 1",
        "  syscall",
        "  li $v0, 4",
        "  la $a0, _ret",
        "  syscall",
        "  move $v0, $0",
        "  jr $ra"
    };
    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
        fputs(lines[i], fp);
        fputc('\n', fp);
    }
}

void printObjectCode(char *filepath, InterCode ir)
{
    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        printf("Cannot open file %s", filepath);
        return;
    }
    initRegisters();
    initFrames(ir);
    initObjectCode(fp);
    InterCode code = ir;
    while (code) {
        switch (code->kind) {
            case CALL_: {
                InterCode arg = code->prev;
                int argCount = 0, arg_reg[18];
                while (arg && arg->kind == ARG_) {
                    int regno = handleOp(fp, arg->ops[0], true);
                    if (++argCount <= 4)
                        fprintf(fp, "  move %s, %s\n", REG_NAME[argCount + 3], REG_NAME[regno]);
                    else
                        arg_reg[argCount - 5] = regno;
                    arg = arg->prev;
                }
                if (argCount > 4)
                    fprintf(fp, "  addi $sp, $sp, %d\n", -4 * (argCount - 4));
                for (int i = argCount - 5; i >= 0; i--)
                    fprintf(fp, "  sw %s, %d($sp)\n", REG_NAME[arg_reg[i]], i * 4);

                char *realName = changename(code->ops[1]->name);
                fprintf(fp, "  addi $sp, $sp, -4\n  sw $ra, 0($sp)\n  jal %s\n", realName);
                fprintf(fp, "  lw $ra, 0($sp)\n  addi $sp, $sp, 4\n");

                updateFramePointer(currentFrame->name);
                if (argCount > 4)
                    fprintf(fp, "  addi $sp, $sp, %d\n", 4 * (argCount - 4));

                Operand ret = code->ops[0];
                if (ret->kind == VARIABLE || ret->kind == TEMP_VAR)
                    fprintf(fp, "  sw $v0, %d($fp)\n", -handleVariable(ret, currentFrame)->offset);
                else if (ret->kind == GET_VAL)
                    fprintf(fp, "  sw $v0, 0(%s)\n", REG_NAME[handleOp(fp, ret->opr, true)]);
                break;
            }
            case FUNC_: {
                char *realName = changename(code->ops[0]->name);
                updateFramePointer(code->ops[0]->name);
                fprintf(fp, "%s:\n  addi $sp, $sp, -4\n  sw $fp, 0($sp)\n  move $fp, $sp\n", realName);
                if (currentFrame->varlist)
                    fprintf(fp, "  addi $sp, $sp, %d\n", -currentFrame->varlist->offset);
                clearRegisters();

                InterCode param = code->next;
                for (int i = 1; param && param->kind == PARAM_; i++, param = param->next) {
                    Variable var = handleVariable(param->ops[0], currentFrame);
                    if (i <= 4)
                        fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[i + 3], -var->offset);
                    else
                        fprintf(fp, "  lw $t0, %d($fp)\n  sw $t0, %d($fp)\n", 4 * (i - 4) + 4, -var->offset);
                }
                break;
            }
            case RETURN_: {
                Operand r = code->ops[0];
                int regno = (r->kind == GET_VAL) ? handleOp(fp, r->opr, true) : handleOp(fp, r, true);
                if (r->kind == GET_VAL)
                    fprintf(fp, "  lw $v0, 0(%s)\n", REG_NAME[regno]);
                else
                    fprintf(fp, "  move $v0, %s\n", REG_NAME[regno]);
                fprintf(fp, "  move $sp, $fp\n  lw $fp, 0($fp)\n  addi $sp, $sp, 4\n  jr $ra\n");
                break;
            }
            case READ_:
                fprintf(fp, "  addi $sp, $sp, -4\n  sw $ra, 0($sp)\n  jal read\n");
                fprintf(fp, "  lw $ra, 0($sp)\n  addi $sp, $sp, 4\n");
                {
                    Operand ret = code->ops[0];
                    if (ret->kind == VARIABLE || ret->kind == TEMP_VAR) {
                        int regno = handleOp(fp, ret, false);
                        fprintf(fp, "  move %s, $v0\n  sw %s, %d($fp)\n", REG_NAME[regno], REG_NAME[regno], -regs[regno]->var->offset);
                    } else if (ret->kind == GET_VAL) {
                        int regno = handleOp(fp, ret->opr, true);
                        fprintf(fp, "  sw $v0, 0(%s)\n", REG_NAME[regno]);
                    }
                }
                break;
            case WRITE_: {
                int regno = handleOp(fp, code->ops[0], true);
                fprintf(fp, "  move $a0, %s\n  addi $sp, $sp, -4\n  sw $ra, 0($sp)\n  jal write\n", REG_NAME[regno]);
                fprintf(fp, "  lw $ra, 0($sp)\n  addi $sp, $sp, 4\n");
                break;
            }
            case ASSIGN_: {
                Operand l = code->ops[0], r = code->ops[1];
                int rreg = handleOp(fp, r, true);
                if (l->kind == VARIABLE || l->kind == TEMP_VAR)
                    fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[rreg], -handleVariable(l, currentFrame)->offset);
                else if (l->kind == GET_VAL)
                    fprintf(fp, "  sw %s, 0(%s)\n", REG_NAME[rreg], REG_NAME[handleOp(fp, l->opr, true)]);
                break;
            }
            case PLUS_: case SUB_: case MULTI_: case DIV_: {
                char *opstr = code->kind == PLUS_ ? "add" : code->kind == SUB_ ? "sub" : code->kind == MULTI_ ? "mul" : "div";
                Operand l = code->ops[0];
                int r1 = handleOp(fp, code->ops[1], true), r2 = handleOp(fp, code->ops[2], true);
                if (l->kind == VARIABLE || l->kind == TEMP_VAR) {
                    int r0 = handleOp(fp, l, false);
                    if (code->kind == DIV_) {
                        fprintf(fp, "  div %s, %s\n  mflo %s\n", REG_NAME[r1], REG_NAME[r2], REG_NAME[r0]);
                    } else {
                        fprintf(fp, "  %s %s, %s, %s\n", opstr, REG_NAME[r0], REG_NAME[r1], REG_NAME[r2]);
                    }
                    fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[r0], -regs[r0]->var->offset);
                } else if (l->kind == GET_VAL) {
                    int reg = handleOp(fp, l->opr, true);
                    if (code->kind == DIV_) {
                        int r3 = handleOp(fp, l->opr, false);
                        fprintf(fp, "  div %s, %s\n  mflo %s\n", REG_NAME[r1], REG_NAME[r2], REG_NAME[r3]);
                        fprintf(fp, "  sw %s, 0(%s)\n", REG_NAME[r3], REG_NAME[reg]);
                    } else {
                        fprintf(fp, "  %s %s, %s, %s\n  sw %s, 0(%s)\n", opstr, REG_NAME[r1], REG_NAME[r1], REG_NAME[r2], REG_NAME[r1], REG_NAME[reg]);
                    }
                }
                break;
            }
            case MEM_: {
                int r0 = handleOp(fp, code->ops[0], true), r1 = handleOp(fp, code->ops[1], true);
                fprintf(fp, "  sw %s, 0(%s)\n", REG_NAME[r1], REG_NAME[r0]);
                break;
            }
            case LABEL_:
                fprintf(fp, "label%d:\n", code->ops[0]->no);
                break;
            case GOTO_:
                fprintf(fp, "  j label%d\n", code->ops[0]->no);
                break;
            case IF_GOTO_: {
                int r0 = handleOp(fp, code->ops[0], true), r1 = handleOp(fp, code->ops[1], true), lbl = code->ops[2]->no;
                char *rel = code->relop;
                if (!strcmp(rel, "==")) fprintf(fp, "  beq %s, %s, label%d\n", REG_NAME[r0], REG_NAME[r1], lbl);
                else if (!strcmp(rel, "!=")) fprintf(fp, "  bne %s, %s, label%d\n", REG_NAME[r0], REG_NAME[r1], lbl);
                else if (!strcmp(rel, ">")) fprintf(fp, "  bgt %s, %s, label%d\n", REG_NAME[r0], REG_NAME[r1], lbl);
                else if (!strcmp(rel, "<")) fprintf(fp, "  blt %s, %s, label%d\n", REG_NAME[r0], REG_NAME[r1], lbl);
                else if (!strcmp(rel, ">=")) fprintf(fp, "  bge %s, %s, label%d\n", REG_NAME[r0], REG_NAME[r1], lbl);
                else if (!strcmp(rel, "<=")) fprintf(fp, "  ble %s, %s, label%d\n", REG_NAME[r0], REG_NAME[r1], lbl);
                break;
            }
            default:
                break;
        }
        fputs("\n", fp);
        fflush(fp);
        code = code->next;
        freeRegisters();
    }
    fclose(fp);
}
