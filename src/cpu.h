#ifndef _H_CPU
#define _H_CPU
#include "mem.h"
#include <string>
using namespace std;

struct Opcode
{
    size_t opcode;
    size_t opint;
    string hex;
    string op;
    size_t adm;
    size_t cycle;
};

class Cpu {
  public:
    uint32_t imgdata[560 * 2 * 192]{};
    bool     imgok = false;

    uint8_t  a;
    uint8_t  x;
    uint8_t  y;
    uint8_t  sp;
    uint16_t pc;

    bool negative;
    bool overflow;
    bool decimal;
    bool interrupt;
    bool zero;
    bool carry;

    uint8_t toirq;
    size_t  cpuclock;
    size_t  cycles;
    size_t  total;

    size_t steps;
    size_t totalcycle;
    Mem   *mem;

    bool cpu_running = false;

  private:
    Opcode opcodes[258];

  public:
    Cpu();
    ~Cpu();

    void init();
    void exec_nmi();
    void exec_irq();

    void step();
    int  run(bool cputest);

    void      draw_frame();
    void      set_img_data(uint8_t r, uint8_t g, uint8_t b, int idx);
    void      clear_img();
    bool      get_img_status();
    uint32_t *get_img_data();

    void reset();
    void clear_cpucycle();

  private:
    void create_opcode(size_t opcode, size_t opint, string hex, string op, size_t adm, size_t cycle);
    void create_opcodes();

    uint16_t get_addr(int mode);
    void     exe_instruction(size_t opint, uint16_t addr);

    void    set_zero_and_ng(uint8_t rval);
    void    setp(uint8_t value);
    uint8_t getp(bool bFlag);
    void    doBranch(bool test, uint16_t reladr);

    void show_state(uint16_t pc, string op, uint16_t adrm);
    void show_test_state(uint16_t pc, string op, uint16_t adrm);
};

#endif
