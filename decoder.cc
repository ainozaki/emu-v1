#include "decoder.h"

#include <cassert>
#include <stdio.h>

#include "arm-op.h"
#include "arm.h"
#include "utils.h"

/* Data Processing Immediate */
const decode_func decode_data_processing_imm_tbl[] = {
    decode_pc_rel,      decode_pc_rel,
    decode_add_sub_imm, decode_add_sub_imm_with_tags,
    decode_logical_imm, decode_move_wide_imm,
    decode_bitfield,    decode_extract,
};

void decode_data_processing_imm(uint32_t inst, Cpu *cpu) {
  uint8_t op0;
  op0 = bitutil::shift(inst, 23, 25);
  decode_data_processing_imm_tbl[op0](inst, cpu);
}

void decode_add_sub_imm(uint32_t inst, Cpu *cpu) {
  uint8_t rd, rn;
  uint16_t imm;
  bool shift, setflag, if_sub, if_64bit;
  uint64_t result;

  rd = bitutil::shift(inst, 0, 4);
  rn = bitutil::shift(inst, 5, 9);
  imm = bitutil::shift(inst, 10, 21);
  shift = bitutil::bit(inst, 22);
  setflag = bitutil::bit(inst, 29);
  if_sub = bitutil::bit(inst, 30);
  if_64bit = bitutil::bit(inst, 31);

  if (shift) {
    imm <<= 12;
  }

  if (setflag) {
    if (if_sub) {
      /* SUBS */
      result = add_imm_s(cpu->xregs[rn], ~imm, /*carry-in=*/1, cpu->cpsr);
    } else {
      /* ADDS */
      result = add_imm_s(cpu->xregs[rn], imm, /*carry-in=*/0, cpu->cpsr);
    }
  } else {
    if (if_sub) {
      /* SUB */
      result = add_imm(cpu->xregs[rn], ~imm, /*carry-in=*/1);
    } else {
      /* ADD */
      result = add_imm(cpu->xregs[rn], imm, /*carry-in=*/0);
    }
  }

  if (if_64bit) {
    cpu->xregs[rd] = result;
  } else {
    cpu->xregs[rd] = bitutil::clear_upper32(result);
  }
}

void decode_add_sub_imm_with_tags(uint32_t inst, Cpu *cpu){};

void decode_logical_imm(uint32_t inst, Cpu *cpu) {
  uint8_t rd, rn, imms, immr, opc;
  bool N, if_64bit;
  uint32_t imm;
  uint64_t result;

  rd = bitutil::shift(inst, 0, 4);
  rn = bitutil::shift(inst, 5, 9);
  imms = bitutil::shift(inst, 10, 15);
  immr = bitutil::shift(inst, 16, 21);
  N = bitutil::bit(inst, 22);
  opc = bitutil::shift(inst, 29, 30);
  if_64bit = bitutil::bit(inst, 31);

  imm = imms; // TODO: implement DecodeBitMasks

  if (!if_64bit && N != 0) {
    fprintf(stderr, "undefined\n");
  }

  switch (opc) {
  case 0b00:
  case 0b11:
    result = cpu->xregs[rn] & imm; /* AND */
    break;
  case 0b01:
    result = cpu->xregs[rn] | imm; /* ORR */
    break;
  case 0b10:
    result = cpu->xregs[rn] ^ imm; /* EOR */
    break;
  default:
    assert(false);
  }

  if (if_64bit) {
    cpu->xregs[rd] = result;
  } else {
    cpu->xregs[rd] = bitutil::clear_upper32(result);
  }
}

void decode_move_wide_imm(uint32_t inst, Cpu *cpu){};
void decode_bitfield(uint32_t inst, Cpu *cpu){};
void decode_extract(uint32_t inst, Cpu *cpu){};

void decode_pc_rel(uint32_t inst, Cpu *cpu){};
void decode_sme_encodings(uint32_t inst, Cpu *cpu){};
void decode_unallocated(uint32_t inst, Cpu *cpu){};
void decode_sve_encodings(uint32_t inst, Cpu *cpu){};
void decode_loads_and_stores(uint32_t inst, Cpu *cpu){};
void decode_data_processing_reg(uint32_t inst, Cpu *cpu){};
void decode_data_processing_float(uint32_t inst, Cpu *cpu){};
void decode_branches(uint32_t inst, Cpu *cpu){};
