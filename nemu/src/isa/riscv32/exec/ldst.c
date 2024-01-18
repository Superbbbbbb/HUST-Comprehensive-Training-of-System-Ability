#include "cpu/exec.h"

make_EHelper(ld) {
  rtl_lm(&s0, &id_src->addr, decinfo.width);
  switch (decinfo.width) {
    case 4: print_asm_template2(lw); break;
    case 2:
      if (decinfo.isa.instr.funct3 >> 2) {
        print_asm_template2(lhu);
      } else {
        rtl_sext(&s0, &s0, 2);
        print_asm_template2(lh);            
      }
      break;
    case 1:
      if (decinfo.isa.instr.funct3 >> 2) {
        print_asm_template2(lbu);   
      } else {
        rtl_sext(&s0, &s0, 1);
        print_asm_template2(lb);
      }
      break;
  }
  rtl_sr(id_dest->reg, &s0, 4);
}

make_EHelper(st) {
  rtl_sm(&id_src->addr, &id_dest->val, decinfo.width);

  switch (decinfo.width) {
    case 4: print_asm_template2(sw); break;
    case 2: print_asm_template2(sh); break;
    case 1: print_asm_template2(sb); break;
    default: assert(0);
  }
}
