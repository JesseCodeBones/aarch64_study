#include <gtest/gtest.h>
#include <functional>
#include <vector>
#include <sys/mman.h>
#include <cstring>
#include "../src/Assembly.hpp"

int callAssembly(std::function<int()> fun){
  return fun();
}


void test() {
  std::cout << "hello JIT\n";
}

uint64_t mov64_x0(){
  asm("mov X0, #0x2211");
  asm("movk X0, #0x4433, lsl #0x10");
  asm("movk X0, #0x6655, lsl #0x20");
  asm("movk X0, #0x8877, lsl #0x30");
  asm("ret");
}

TEST(assembly_test, base_lambda) {
  ASSERT_EQ(42, callAssembly([]()->int{return 42;}));
}

std::function<int()> createJit(const std::vector<uint8_t>& executable){
  void *jitPtr = mmap(nullptr, 4096, PROT_READ | PROT_EXEC | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  std::memcpy(jitPtr, executable.data(), executable.size());
  return (int(*)())jitPtr;
}

std::function<uint64_t()> createJit64(const std::vector<uint8_t>& executable){
  void *jitPtr = mmap(nullptr, 4096, PROT_READ | PROT_EXEC | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  std::memcpy(jitPtr, executable.data(), executable.size());
  return (uint64_t(*)())jitPtr;
}

std::function<void()> createJitVoid(const std::vector<uint8_t>& executable){
  void *jitPtr = mmap(nullptr, 4096, PROT_READ | PROT_EXEC | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  std::memcpy(jitPtr, executable.data(), executable.size());
  return (void(*)())jitPtr;
}




/// @brief https://developer.arm.com/documentation/ddi0596/2021-12/Base-Instructions/MOV--wide-immediate---Move--wide-immediate---an-alias-of-MOVZ-?lang=en
TEST(assembly_test, base_jit_mov) {
  /**
  32-bit variant
    Applies when sf == 0.
    ADD <Wd|WSP>, <Wn|WSP>, <Wm>{, <extend> {#<amount>}}
  64-bit variant
    Applies when sf == 1.
  */
  std::vector<uint8_t> assemblies;
  /// @brief OPCode MOV IMM16
  /// -> 0b10100101 是mov IMM16的opcode
  /// 具体各位置查看上面的链接
  uint32_t mov42 = 0b10100101 << 23;
  /// 0b101010 = 42，将其右移5位填入IMM16位置
  /// RD位置空置为0，因为就是往w0位置存放返回值
  mov42 |= (0b101010 << 5);
  /// <-

  /// @brief OPCode RET
  /// -> 0xD65F0000 是RET的opcode
  /// 30的意思是X30寄存器，这个寄存器是LR寄存器，后续的跳转回来需要用
  uint32_t p = 0xD65F0000 | (30 << 5);
  /// <-
  addUint32_t(assemblies, mov42);
  addUint32_t(assemblies, p);
  auto fun = createJit(assemblies);
  uint32_t result = fun();
  ASSERT_EQ(42, fun());
}

TEST(assembly_test, base_jit_mov_8877665544332211) {
  // 函数栈帧的保存
  /// @brief https://developer.arm.com/documentation/ddi0596/2021-12/Base-Instructions/STP--Store-Pair-of-Registers-
  // stp x29, x30, [sp, #-16]!  <- 0xa9bf7bfd
  uint32_t stp_x29_x30 = 0b101010011 << 23;
  stp_x29_x30 |= (0x2211 << 5);


  /// @brief 特殊用途寄存器编号
  // X29 FP
  // X30 LR RET
  // X31 SP
  
  /// 关于IMM7的文档：
  // <imm>	
  // For the 32-bit post-index and 32-bit pre-index variant: is the signed immediate byte offset, a multiple of 4 in the range -256 to 252, encoded in the "imm7" field as <imm>/4.
  // For the 32-bit signed offset variant: is the optional signed immediate byte offset, a multiple of 4 in the range -256 to 252, defaulting to 0 and encoded in the "imm7" field as <imm>/4.
  // For the 64-bit post-index and 64-bit pre-index variant: is the signed immediate byte offset, a multiple of 8 in the range -512 to 504, encoded in the "imm7" field as <imm>/8.
  // For the 64-bit signed offset variant: is the optional signed immediate byte offset, a multiple of 8 in the range -512 to 504, defaulting to 0 and encoded in the "imm7" field as <imm>/8.
  stp_x29_x30 |= (((-16 / 8) & 0b1111111) << 15); // IMM7
  stp_x29_x30 |= (30 << 10); // RT2
  stp_x29_x30 |= (31 << 5); // RN sp = X31
  stp_x29_x30 |= (29); // RT

  /// call (mov pc, ptr)
  /// IMM16:hw的计算方式: ~(IMM16 << hw )
  uintptr_t ptr = (uintptr_t)test;
  // std::cout << std::hex << ptr << std::endl;
  // uint64_t val = mov64_x0();
  // std::cout << "value=" << std::hex << val << std::endl;

  /// @brief mov 0x8877665544332211 to X0
  uint32_t mov_x0_2211 = 0b10100101 << 23;
  mov_x0_2211 |= (0x2211 << 5);
  /// MOVK Document: https://developer.arm.com/documentation/ddi0596/2021-12/Base-Instructions/MOVK--Move-wide-with-keep-?lang=en
  uint32_t mov_x0_4433 = 0b111100101 << 23;
  mov_x0_4433 |= (0x4433 << 5);
  // hw移位定义
  //   <shift>	
  // For the 32-bit variant: is the amount by which to shift the immediate left, either 0 (the default) or 16, encoded in the "hw" field as <shift>/16.
  // For the 64-bit variant: is the amount by which to shift the immediate left, either 0 (the default), 16, 32 or 48, encoded in the "hw" field as <shift>/16.
  mov_x0_4433 |= (1 << 21);
  uint32_t mov_x0_6655 = 0b111100101 << 23;
  mov_x0_6655 |= (0x6655 << 5);
  mov_x0_6655 |= (2 << 21);
  uint32_t mov_x0_8877 = 0b111100101 << 23;
  mov_x0_8877 |= (0x8877 << 5);
  mov_x0_8877 |= (3 << 21);

  uint32_t ret = 0xD65F0000 | (30 << 5);

  /// @brief LDP 
  uint32_t LDP_X29_X30 = 0b1010100011 << 22;
  LDP_X29_X30 |= (((16 / 8) & 0b1111111) << 15); // IMM7
  LDP_X29_X30 |= (30 << 10); // RT2
  LDP_X29_X30 |= (31 << 5); // RN sp = X31
  LDP_X29_X30 |= (29); // RT
  // ldp x29, x30, [sp], #16    <- 0xa8c17bfd

  // create JIT function
   std::vector<uint8_t> assemblies;
   addUint32_t(assemblies, stp_x29_x30);
   addUint32_t(assemblies, mov_x0_2211);
   addUint32_t(assemblies, mov_x0_4433);
   addUint32_t(assemblies, mov_x0_6655);
   addUint32_t(assemblies, mov_x0_8877);
   addUint32_t(assemblies, LDP_X29_X30);
   addUint32_t(assemblies, ret);
   auto fun = createJit64(assemblies);
   ASSERT_EQ(0x8877665544332211, fun());
 }

  void helloJesse(){
    std::cout << "hello Jesse\n";
  }

 TEST(assembly_test, base_jit_call_native) { 

  std::vector<uint8_t> assemblies;

  uint32_t stp_x29_x30 = 0b101010011 << 23;
  stp_x29_x30 |= (0x2211 << 5);
  stp_x29_x30 |= (((-16 / 8) & 0b1111111) << 15); // IMM7
  stp_x29_x30 |= (30 << 10); // RT2
  stp_x29_x30 |= (31 << 5); // RN sp = X31
  stp_x29_x30 |= (29); // RT
  addUint32_t(assemblies, stp_x29_x30);


  void* printFun = (void*) helloJesse;
  auto functionToReg = insertPtrToRegister(1, printFun);
  for(auto code : functionToReg) {
    assemblies.push_back(code);
  }

  /// @brief BLR: https://developer.arm.com/documentation/ddi0596/2021-12/Base-Instructions/BLR--Branch-with-Link-to-Register-?lang=en 
  uint32_t call_x1 = 0b1101011000111111000000 << 10;
  call_x1 |= (1 << 5); // register << 5
  addUint32_t(assemblies, call_x1);

  uint32_t LDP_X29_X30 = 0b1010100011 << 22;
  LDP_X29_X30 |= (((16 / 8) & 0b1111111) << 15); // IMM7
  LDP_X29_X30 |= (30 << 10); // RT2
  LDP_X29_X30 |= (31 << 5); // RN sp = X31
  LDP_X29_X30 |= (29); // RT
  uint32_t ret = 0xD65F0000 | (30 << 5);
  addUint32_t(assemblies, LDP_X29_X30);
  addUint32_t(assemblies, ret);

  auto fun = createJitVoid(assemblies);
  fun();
 }