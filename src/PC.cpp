#include "PC.h"
#include "cpu.h"

PC::PC()
{
    cpu = new Cpu();
}
PC::~PC()
{
    delete cpu;
}
void PC::init()
{
    cpu->init();
    load_bios("rom/Apple2e.rom");
}
void PC::load_bios(string path)
{
    cpu->mem->set_bin(path, true);
}
void PC::load_prg(string path)
{
    cpu->mem->set_bin(path, false);
    cpu->pc = cpu->mem->prg_offset;
}
void PC::start()
{
    cpu->cpu_running = true;
}
void PC::tick()
{
    if (cpu->cpu_running) {
        cpu->step();
    }
}
