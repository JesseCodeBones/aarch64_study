#include <gtest/gtest.h>
#include <functional>
#include <vector>
#include <sys/mman.h>
#include <cstring>
int callAssembly(std::function<int()> fun){
  return fun();
}

int return42(int a){
  return a + 42;
}

void test() {
  std::cout << "Hello JIT\n";
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

void addUint32_t(std::vector<uint8_t>& executable, uint32_t value){
  executable.push_back(value & 0xff);
  executable.push_back((value >> (sizeof(uint8_t) * 8)) & 0xff);
  executable.push_back((value >> ((2*sizeof(uint8_t)) * 8)) & 0xff);
  executable.push_back((value >> ((3*sizeof(uint8_t)) * 8)) & 0xff);
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

uint32_t immNeg(uint32_t positive, uint32_t length){
  positive = ~positive;
  uint32_t mask = 0;
  for(int i = 0; i < length; ++i) {
    mask = (mask | (1 << i));
  }
  positive &= mask;
  return positive;
}

TEST(assembly_test, base_jit_call) {
  // 函数栈帧的保存
  /// @brief https://developer.arm.com/documentation/ddi0596/2021-12/Base-Instructions/STP--Store-Pair-of-Registers-
  // stp x29, x30, [sp, #-16]!  <- 0xa9bf7bfd
  uint32_t stp_x29_x30 = 0b101010011 << 23;
  uint32_t neg = immNeg(16, 7);
  
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
  std::cout << std::hex << ptr << std::endl;

  uint32_t movPtr = 0b100100101;

  /// @brief LDP 
  uint32_t LDP_X29_X30 = 0b1010100011 << 22;
  LDP_X29_X30 |= (((16 / 8) & 0b1111111) << 15); // IMM7
  LDP_X29_X30 |= (30 << 10); // RT2
  LDP_X29_X30 |= (31 << 5); // RN sp = X31
  LDP_X29_X30 |= (29); // RT
  std::cout << std::hex << stp_x29_x30 << "-" << LDP_X29_X30 << std::endl;
  // ...call
  // ldp x29, x30, [sp], #16    <- 0xa8c17bfd
  test();
 }