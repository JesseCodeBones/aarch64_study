#ifndef __jesse_asm_assembly__
#define __jesse_asm_assembly__

#include <vector>
#include <cstdint>

static void addUint32_t(std::vector<uint8_t>& executable, const uint32_t value){
  executable.push_back(value & 0xff);
  executable.push_back((value >> (sizeof(uint8_t) * 8)) & 0xff);
  executable.push_back((value >> ((2*sizeof(uint8_t)) * 8)) & 0xff);
  executable.push_back((value >> ((3*sizeof(uint8_t)) * 8)) & 0xff);
}

static std::vector<uint8_t> insertPtrToRegister(const uint8_t registerIndex, void const*const ptr) {

  const uintptr_t value = (uintptr_t)ptr;
  std::vector<uint8_t> asms;

  uint32_t mov_x_2 = 0b10100101 << 23;
  mov_x_2 |= ((value & 0xffff) << 5);
  mov_x_2 |= registerIndex;
  uint32_t mov_x_4 = 0b111100101 << 23;
  mov_x_4 |= (((value >> 16) & 0xffff) << 5);
  mov_x_4 |= registerIndex;
  mov_x_4 |= (1 << 21);
  uint32_t mov_x_6 = 0b111100101 << 23;
  mov_x_6 |= (((value >> 32) & 0xffff) << 5);
  mov_x_6 |= registerIndex;
  mov_x_6 |= (2 << 21);
  uint32_t mov_x_8 = 0b111100101 << 23;
  mov_x_8 |= (((value >> 48) & 0xffff) << 5);
  mov_x_8 |= (3 << 21);
  addUint32_t(asms, mov_x_2);
  addUint32_t(asms, mov_x_4);
  addUint32_t(asms, mov_x_6);
  addUint32_t(asms, mov_x_8);
  return asms;
}

#endif