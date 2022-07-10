#include "mem.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>

Mem::Mem()
{
    for (int i = 0; i < 256; i++) {
        color_lut[i] = new uint8_t[4 * 7 * 2]{};
    }
    make_color_lut();
}
Mem::~Mem()
{
    for (int i = 0; i < 256; i++) {
        delete[] color_lut[i];
    }
    clear_bios();
    clear_prg();
}
void Mem::init()
{
    reset();
}
void Mem::make_color_lut()
{
    for (int c = 0; c < 256; c++) {
        int dst = 0;
        for (int i = 0; i < 7; i++) {
            uint8_t p           = ((c >> i) & 1) ? 0xff : 00;
            color_lut[c][dst++] = 0x00;
            color_lut[c][dst++] = p;
            color_lut[c][dst++] = 0x00;
            color_lut[c][dst++] = 0xff;
            color_lut[c][dst++] = 0x00;
            color_lut[c][dst++] = p;
            color_lut[c][dst++] = 0x00;
            color_lut[c][dst++] = 0xff;
        }
    }
}
uint8_t Mem::get(uint16_t addr)
{
    if (addr > 0xc000) {
        switch (addr) {
            case 0xc01c:
                return page_2;

            case 0xc054:
                page_2 = 0;
                break;

            case 0xc055:
                page_2 = 1;
                break;
        }
    }
    return ram[addr];
}
void Mem::set(uint16_t addr, uint8_t data)
{
    if (addr >= 0x2000 && addr < 0x6000) {
        int scanline              = offset_to_scanline[addr - 0x2000];
        dirty_scanlines[scanline] = 1;
    } else if (addr > 0xc000) {
        switch (addr) {
            case 0xc054:
                page_2 = 0;
                break;

            case 0xc055:
                page_2 = 1;
                break;
        }
    }
    ram[addr] = data;
}
uint16_t Mem::get16(uint16_t addr)
{
    uint16_t l = get(addr);
    uint16_t h = get(addr + 1);
    uint16_t r = l | (h << 8);
    return r;
}
void Mem::set_bin(string filename, bool bios)
{

    FILE *f = fopen(filename.c_str(), "rb");
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (bios) {
        bios_len = size;
        bios_rom = new uint8_t[bios_len];
        auto _   = fread(bios_rom, bios_len, 1, f);
        load_bios();
    } else {
        prg_len = size;
        prg_rom = new uint8_t[prg_len];
        auto _  = fread(prg_rom, prg_len, 1, f);
        load_prg();
    }
    fclose(f);
}
void Mem::load_bios()
{
    clear_bios();
    for (int i = 0; i < bios_len; i++) {
        ram[0xc000 + i] = bios_rom[i] & 0xff;
    }
}
void Mem::clear_bios()
{
    if (bios_rom != nullptr)
        delete bios_rom;
}
void Mem::load_prg()
{
    clear_prg();
    prg_offset = ((prg_rom[0] & 0xff) | ((prg_rom[1] & 0xff) << 8));
    prg_len    = ((prg_rom[2] & 0xff) | ((prg_rom[3] & 0xff) << 8));
    int ptr    = prg_offset;

    for (size_t i = 4; i < prg_len + 4; i++) {
        ram[ptr++] = prg_rom[i] & 0xff;
    }
}
void Mem::clear_prg()
{
    if (prg_rom != nullptr)
        delete prg_rom;
}
void Mem::reset()
{
    clear_bios();
    clear_prg();
    for (size_t i = 0; i < 0xffff; i++) {
        ram[i] = 0;
    }

    for (size_t i = 0; i < 0x2000 * 2; i++) {
        offset_to_scanline[i] = 0;
    }

    for (size_t row = 0; row < 192; row++) {
        size_t src = ((row & 7) * 0x400) + (((row >> 3) & 0x07) * 0x80) + (((row >> 6) & 0x7) * 0x28);

        for (size_t x = 0; x < 40; x++) {
            offset_to_scanline[src + x]          = row;
            offset_to_scanline[0x2000 + src + x] = (row + 192);
        }
    }

    for (size_t i = 0; i < 2 * 192; i++) {
        dirty_scanlines[i] = 1;
    }
}
