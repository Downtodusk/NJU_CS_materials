#include "x86.h"
#include "device.h"

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

void GProtectFaultHandle(struct StackFrame *sf);
void timerHandle(struct StackFrame *sf);

void syscallHandle(struct StackFrame *sf);

void syscallWrite(struct StackFrame *sf);
void syscallPrint(struct StackFrame *sf);
void syscallFork(struct StackFrame *sf);
void syscallSleep(struct StackFrame *sf);
void syscallExit(struct StackFrame *sf);

//help func
inline void switch_process() {
	//switch process
	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].stackTop = pcb[current].prevStackTop;
	tss.esp0 = (uint32_t)&(pcb[current].stackTop);
	asm volatile("movl %0, %%esp"::"m"(tmpStackTop)); // switch kernel stack
	asm volatile("popl %gs");
	asm volatile("popl %fs");
	asm volatile("popl %es");
	asm volatile("popl %ds");
	asm volatile("popal");
	asm volatile("addl $8, %esp");
	asm volatile("iret");
}

inline void search_next_process() {
	int next_pid = (current + 1) % MAX_PCB_NUM;
	while(next_pid != current) {
		if(pcb[next_pid].state == STATE_RUNNABLE && next_pid != 0) {
			current = next_pid;
			break;
		}
		next_pid = (next_pid + 1) % MAX_PCB_NUM;
	}
	if(next_pid == current) {
		current = 0;
		pcb[current].state = STATE_RUNNING;
	}
}

void irqHandle(struct StackFrame *sf)
{ // pointer sf = esp
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds" ::"a"(KSEL(SEG_KDATA)));
	/*XXX Save esp to stackTop */
	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)sf;

	switch (sf->irq)
	{
	case -1:
		break;
	case 0xd:
		GProtectFaultHandle(sf);
		break;
	case 0x20:
		timerHandle(sf);
		break;
	case 0x80:
		syscallHandle(sf);
		break;
	default:
		assert(0);
	}
	/*XXX Recover stackTop */
	pcb[current].stackTop = tmpStackTop;
}

void GProtectFaultHandle(struct StackFrame *sf)
{
	assert(0);
	return;
}

void timerHandle(struct StackFrame *sf)
{
	for(int i = 0; i < MAX_PCB_NUM; i++) {
		if(pcb[i].state == STATE_BLOCKED) {
			if(pcb[i].sleepTime > 0) {
				pcb[i].sleepTime -= 1;
			}
			if(pcb[i].sleepTime == 0) {
				pcb[i].state = STATE_RUNNABLE;
			}
		}
	}
	if(pcb[current].timeCount < MAX_TIME_COUNT && pcb[current].state == STATE_RUNNING) {
		pcb[current].timeCount += 1;
		return;	
	}
	pcb[current].timeCount = 0;
	pcb[current].state = STATE_RUNNABLE;
	int nxt_pid = (current + 1) % MAX_PCB_NUM;
	while(nxt_pid != current) {
		if (pcb[nxt_pid].state == STATE_RUNNABLE && nxt_pid != 0) {
			current = nxt_pid;
			break;
		}
		nxt_pid = (nxt_pid + 1) % MAX_PCB_NUM;
	}
	pcb[current].state = STATE_RUNNING;
	switch_process();
}

void syscallHandle(struct StackFrame *sf)
{
	switch (sf->eax)
	{ // syscall number
	case 0:
		syscallWrite(sf);
		break; // for SYS_WRITE
	/*TODO Add Fork,Sleep... */
	case 1:
		syscallFork(sf);
		break;
	case 3:
		syscallSleep(sf);
		break;
	case 4:
		syscallExit(sf);
		break;
	default:
		break;
	}
}

void syscallWrite(struct StackFrame *sf)
{
	switch (sf->ecx)
	{ // file descriptor
	case 0:
		syscallPrint(sf);
		break; // for STD_OUT
	default:
		break;
	}
}

void syscallPrint(struct StackFrame *sf)
{
	int sel = sf->ds; // segment selector for user data, need further modification
	char *str = (char *)sf->edx;
	int size = sf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es" ::"m"(sel));
	for (i = 0; i < size; i++)
	{
		asm volatile("movb %%es:(%1), %0" : "=r"(character) : "r"(str + i));
		if (character == '\n')
		{
			displayRow++;
			displayCol = 0;
			if (displayRow == 25)
			{
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else
		{
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80)
			{
				displayRow++;
				displayCol = 0;
				if (displayRow == 25)
				{
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
		// asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
		// asm volatile("int $0x20":::"memory"); //XXX Testing irqTimer during syscall
	}

	updateCursor(displayRow, displayCol);
	// take care of return value
	return;
}

// TODO syscallFork ...
void syscallFork(struct StackFrame *sf) {
	int new_pid = 0;
	while(new_pid < MAX_PCB_NUM && pcb[new_pid].state != STATE_DEAD) {
		new_pid++;
	}
	if(new_pid == MAX_PCB_NUM) {
		//进程创建失败，返回-1
		pcb[current].regs.eax = -1;
		return;
	}
	enableInterrupt();
	for (int i = 0; i < 0x100000; i++) {
		*(uint8_t *)(i + (new_pid + 1) * 0x100000) = *(uint8_t *)(i + (current + 1) * 0x100000);
		asm volatile("int $0x20");
	}
	disableInterrupt();
	for(int i = 0; i < MAX_STACK_SIZE; i++) {
		pcb[new_pid].stack[i] = pcb[current].stack[i];
	}
	pcb[new_pid].pid = new_pid;
	pcb[new_pid].state = STATE_RUNNABLE;
	pcb[new_pid].timeCount = 0;
	pcb[new_pid].sleepTime = 0;
	pcb[new_pid].stackTop = (uint32_t)&(pcb[new_pid].regs);
	pcb[new_pid].prevStackTop = (uint32_t)&(pcb[new_pid].stackTop);
	// pcb[new_pid].regs = ?
	pcb[new_pid].regs.cs = USEL(1 + 2*new_pid);
	pcb[new_pid].regs.ds = USEL(2 + 2*new_pid);
	pcb[new_pid].regs.es = USEL(2 + 2*new_pid);
	pcb[new_pid].regs.fs = USEL(2 + 2*new_pid);
	pcb[new_pid].regs.gs = USEL(2 + 2*new_pid);
	pcb[new_pid].regs.ss = USEL(2 + 2*new_pid);
	pcb[new_pid].regs.ebx = pcb[current].regs.ebx;
	pcb[new_pid].regs.ecx = pcb[current].regs.ecx;
	pcb[new_pid].regs.edx = pcb[current].regs.edx;
	pcb[new_pid].regs.ebp = pcb[current].regs.ebp;
	pcb[new_pid].regs.edi = pcb[current].regs.edi;
	pcb[new_pid].regs.esi = pcb[current].regs.esi;
	pcb[new_pid].regs.xxx = pcb[current].regs.xxx;
	pcb[new_pid].regs.irq = pcb[current].regs.irq;
	pcb[new_pid].regs.error = pcb[current].regs.error;
	pcb[new_pid].regs.eip = pcb[current].regs.eip;
	pcb[new_pid].regs.eflags = pcb[current].regs.eflags;
	pcb[new_pid].regs.esp = pcb[current].regs.esp;

	pcb[new_pid].regs.eax = 0;
	pcb[current].regs.eax = new_pid;
}

void syscallSleep(struct StackFrame *sf) {
	pcb[current].sleepTime = sf->ecx;
	pcb[current].state = STATE_BLOCKED;
	//ATTENTION:判断传入参数的合法性
	assert(pcb[current].sleepTime > 0);
	asm volatile("int $0x20");
}

void syscallExit(struct StackFrame *sf) {
	pcb[current].state = STATE_DEAD;
	asm volatile("int $0x20");
}
