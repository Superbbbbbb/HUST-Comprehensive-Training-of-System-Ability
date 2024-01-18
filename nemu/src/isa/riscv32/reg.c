#include "nemu.h"

const char *regsl[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  for (int i = 0; i < 32; i++) {
    printf("%s\t%#x\n", regsl[i], reg_l(i));
  }
}

uint32_t isa_reg_str2val(const char *s, bool *success) {
  int i;
  if(!strcmp(s,"0\0")){
    *success = true;
    return 0;
  }
  for(i=1;i<32;++i){
    if(!strcmp(s,regsl[i])){
      *success = true;
      return cpu.gpr[i]._32;
    }
  }
  return 0;
}

