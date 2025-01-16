#include "x86.h"
#include "device.h"

int displayRow = 0;
int displayCol = 0;
uint16_t displayMem[80*25];
int displayClear = 0;

void initVga() {
	displayRow = 0;
	displayCol = 0;
	displayClear = 0;
	clearScreen();
	updateCursor(0, 0);
}

void clearScreen() {
	int i = 0;
	int pos = 0;
	uint16_t data = 0 | (0x0c << 8);
	for (i = 0; i < 80 * 25; i++) {
		pos = i * 2;  //每个字符占用两个字节
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));  
		/*将变量 data 中的值移动到显示内存中指定位置
		%0 和 %1 分别代表输入操作数的占位符
		对应于 "r"(data) 和 "r"(pos+0xb8000)
		这条语句的作用是将黑色背景色填充到当前字符位置，即清空屏幕上的内容 */
	}
}

void updateCursor(int row, int col){  //更新光标的位置
	int cursorPos = row * 80 + col;  //光标位置的偏移量
	//设置光标位置的低字节
	outByte(0x3d4, 0x0f);
	outByte(0x3d5, (unsigned char)(cursorPos & 0xff));
	//设置光标位置的高字节
	outByte(0x3d4, 0x0e);
	outByte(0x3d5, (unsigned char)((cursorPos>>8) & 0xff));
}

void scrollScreen() {
	int i = 0;
	int pos = 0;
	uint16_t data = 0;
	for (i = 0; i < 80 * 25; i++) {
		pos = i * 2;
		asm volatile("movw (%1), %0":"=r"(data):"r"(pos+0xb8000));
		displayMem[i] = data;
		//put screen in displayMem
	}
	for (i = 0; i < 80 * 24; i++) {
		pos = i * 2;
		data = displayMem[i+80];
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
	}
	data = 0 | (0x0c << 8);
	for (i = 80 * 24; i < 80 * 25; i++) {
		pos = i * 2;
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
	}
}
