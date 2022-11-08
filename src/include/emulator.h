#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <vector>

#include "const.h"
#include "cpu.h"
#include "loader.h"

class Emulator {
public:
  Cpu cpu;
  Loader loader;

  Emulator(int argc, char **argv, char **envp);
  ~Emulator();
  int init();
  void execute_loop();

private:
  char *filename_;
  bool init_done_;
};
