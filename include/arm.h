#pragma once

#include <cstdint>

struct CPSR {
  char buff[27];
  uint8_t Q : 1;
  uint8_t V : 1;
  uint8_t C : 1;
  uint8_t Z : 1;
  uint8_t N : 1;
};

class Cpu {
public:
  Cpu() = default;
  ~Cpu() = default;

public:
  void show_regs();
  void execute(uint32_t inst);

  uint64_t xregs[31] = {0};
  uint64_t pc;
  uint64_t sp_el[4];  /* Stack pointers*/
  uint64_t elr_el[4]; /* Exception Linked Registers */

  CPSR cpsr; /* Current Program Status Register*/
};
