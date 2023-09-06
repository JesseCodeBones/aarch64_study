#ifndef __jesse_asm_asm_function__
#define __jesse_asm_asm_function__

static uint64_t testAsm(){
  // asm("mov X0, #0x2211");
  // asm("movk X0, #0x4433, lsl #0x10");
  // asm("movk X0, #0x6655, lsl #0x20");
  // asm("movk X0, #0x8877, lsl #0x30");

  // asm("str X1, [sp, #-8]!");
  // asm("LDR X0, [sp], #8");
  asm("mov x0, #0");
  asm("mov X1, sp");
  asm("str X1, [sp], #-16");
  asm("ldr x0, [sp, #16]!");
  // asm("add sp, sp, #8");
  asm("ret");
  // a = 10;
  // return a;
}

#endif