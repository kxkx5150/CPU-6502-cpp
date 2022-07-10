#ifndef _H_PC
#define _H_PC
#include "cpu.h"

class PC {
  public:
    Cpu *cpu = nullptr;

  public:
    PC();
    ~PC();

    void init();
    void load_bios(string path);
    void load_prg(string path);

    void start();
    void tick();

  private:
};
#endif
