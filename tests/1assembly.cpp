#include <gtest/gtest.h>
#include <functional>
#include <vector>
#include <sys/mman.h>
#include <cstring>
int callAssembly(std::function<int()> fun){
  return fun();
}

int return42(){
  return 42;
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