#include "cpu.h"
#include "cpu_enum.h"
#include "mem.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>

Cpu::Cpu()
{
    create_opcodes();
    mem = new Mem();
}
Cpu::~Cpu()
{
    delete mem;
}
void Cpu::init()
{
    mem->init();
    reset();
}
void Cpu::reset()
{
    a  = 0;
    x  = 0;
    y  = 0;
    sp = 0xff;
    pc = 0;

    negative  = false;
    overflow  = false;
    decimal   = false;
    interrupt = true;
    zero      = false;
    carry     = false;

    toirq    = 0x00;
    cpuclock = 7;
    cycles   = 0;
    total    = 0;

    totalcycle = 7;
    steps      = 0;

    cpu_running = false;
    imgok       = false;
    mem->reset();
}
void Cpu::exec_nmi()
{
    auto opc = opcodes[256];
    cpuclock += 7;
    exe_instruction(opc.opcode, 0);
}
void Cpu::exec_irq()
{
    auto opc = opcodes[257];
    cpuclock += 7;
    exe_instruction(opc.opcode, 0);
}
void Cpu::step()
{
    int i = 12600;
    while (0 < i) {
        i -= run(false);
    }
    draw_frame();
}
void Cpu::draw_frame()
{
    bool     scanlines = true;
    int      page_2    = mem->page_2;
    int      y0        = (1 - page_2) ? 0 : 192;
    int      src0      = (page_2 ? 0x4000 : 0x2000);
    uint32_t imgidx    = 0;

    for (int y = 0; y < 192; y++) {
        mem->dirty_scanlines[y + y0] = 0;

        int src = src0 + mem->scanline_to_offset[y];
        int dst = y * 1120;
        int idx = 0;

        for (int x = 0; x < 40; x++) {
            size_t c = mem->get(src++);
            for (int i = 0; i < 7 * 8; i += 4) {
                int r = mem->color_lut[c][i];
                int g = mem->color_lut[c][i + 1];
                int b = mem->color_lut[c][i + 2];
                set_img_data(r, g, b, dst + idx);
                set_img_data(r, g, b, dst + 560 + idx);
                idx++;
            }
        }
    }
    imgok = true;
}
bool Cpu::get_img_status()
{
    if (imgok) {
        return true;
    } else {
        return false;
    }
}
void Cpu::clear_img()
{
    imgok = false;
}
uint32_t *Cpu::get_img_data()
{
    return imgdata;
}
void Cpu::set_img_data(uint8_t r, uint8_t g, uint8_t b, int idx)
{
    uint32_t dots = (0xFF000000 | (r << 16) | (g << 8) | b);
    imgdata[idx]  = dots;
}
int Cpu::run(bool cputest)
{
    std::string istr = "";    // irq->check_interrupt(interrupt);
    if (istr == "nmi") {
        // irq->clear_nmi();
        exec_nmi();
    } else if (istr == "irq") {
        // irq->clear_irq();
        exec_irq();
    }

    uint16_t prepc    = pc;
    uint8_t  instr    = mem->get(pc++);
    auto     optobj   = opcodes[instr];
    auto     optcycle = optobj.cycle;
    auto     op       = optobj.op;
    auto     adrm     = get_addr(optobj.adm);

    if (cputest) {
        show_test_state(prepc, op, adrm);
    }
    if (false) {
        show_state(prepc, op, adrm);
    }
    if (steps == 127632) {
        printf(" ");
    }
    exe_instruction(optobj.opcode, adrm);
    steps++;
    return optcycle;
}
void Cpu::clear_cpucycle()
{
    cpuclock = 0;
}
void Cpu::show_state(uint16_t pc, string op, uint16_t adrm)
{
    auto p = getp(false);
    printf("\n");
    printf("pc         : %04X\n", pc);
    printf("opcode     : %s\n", op.c_str());
    printf("regs       : A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", a, x, y, p, sp);
}
void Cpu::show_test_state(uint16_t pc, string op, uint16_t adrm)
{
    // auto p = getp(false);
    // char testchar[200];
    // sprintf (teststr, "%04X", pc);
    // sprintf (teststr, "CYC:%d", totalcycle);
    // sprintf(testchar, "A:%02X X:%02X Y:%02X P:%02X SP:%02X", a, x, y, p, sp);
    // string teststr = testchar;
    // string okstr = NESTEST_ADDR.at(steps);
    // string okstr = NESTEST_CYCLES.at(steps);
    // string okstr = NESTEST_REGS.at(steps);

    // printf("\n");
    // printf("OK : %s\n", okstr.c_str());
    // printf("   : %s\n", teststr.c_str());
    // printf("pc         : %04X\n", pc);
    // printf("opcode     : %s\n", op.c_str());
    // printf("totalcycle : %zu\n", totalcycle);

    // if (teststr != okstr) {
    //     printf("\n\n----------- error -----------\n");
    //     printf("pc    : %04X\n", pc);
    //     printf("op    : %s\n", op.c_str());
    //     printf("steps : %zu\n", steps);
    //     exit(1);
    // }
}
uint16_t Cpu::get_addr(int adm)
{
    switch (adm) {
        case IMP: {
            return 0;
        } break;
        case IMM: {
            return pc++;
        } break;
        case ZP: {
            return mem->get(pc++);
        } break;
        case ZPX: {
            uint16_t adr = mem->get(pc++);
            return ((adr + x) & 0xff);
        } break;
        case ZPY: {
            uint16_t adr = mem->get(pc++);
            return ((adr + y) & 0xff);
        } break;
        case IZX: {
            uint16_t mdata = mem->get(pc++);
            uint16_t adr   = ((mdata + x) & 0xff);
            uint16_t val   = mem->get((adr + 1) & 0xff);
            return (mem->get(adr) | (val << 8));
        } break;
        case IZY: {
            uint16_t adr  = mem->get(pc++);
            uint16_t val  = mem->get((adr + 1) & 0xff);
            uint16_t radr = mem->get(adr) | (val << 8);
            return (radr + y) & 0xffff;
        } break;
        case IZYr: {
            uint16_t adr  = mem->get(pc++);
            uint16_t hval = mem->get((adr + 1) & 0xff);
            uint32_t radr = mem->get(adr) | (hval << 8);
            uint32_t aaa  = radr + y;
            uint32_t bbb  = (aaa >> 8);
            if ((radr >> 8) < (bbb) >> 8) {
                cpuclock += 1;
            }
            return ((radr + y) & 0xffff);
        } break;
        case ABS: {
            uint16_t adr = mem->get(pc++);
            uint16_t val = mem->get(pc++);
            adr |= val << 8;
            return adr;
        } break;
        case ABX: {
            uint16_t adr = mem->get(pc++);
            uint16_t val = mem->get(pc++);
            adr |= val << 8;
            return (adr + x) & 0xffff;
        } break;
        case ABXr: {
            uint16_t adr = mem->get(pc++);
            uint16_t val = mem->get(pc++);
            adr |= val << 8;
            if (adr >> 8 < (adr + x) >> 8) {
                cpuclock += 1;
            }
            return (adr + x) & 0xffff;
        } break;
        case ABY: {
            uint16_t adr = mem->get(pc++);
            uint16_t val = mem->get(pc++);
            adr |= val << 8;
            return (adr + y) & 0xffff;
        } break;
        case ABYr: {
            uint32_t adr = mem->get(pc++);
            uint16_t val = mem->get(pc++);
            adr |= val << 8;
            if (adr >> 8 < (adr + y) >> 8) {
                cpuclock += 1;
            }
            return (adr + y) & 0xffff;
        } break;
        case IND: {
            uint16_t adrl = mem->get(pc++);
            uint16_t adrh = mem->get(pc++);
            uint16_t radr = mem->get(adrl | (adrh << 8));
            uint16_t val  = mem->get(((adrl + 1) & 0xff) | (adrh << 8));
            radr |= val << 8;
            return radr;
        } break;
        case REL: {
            return mem->get(pc++);
        } break;

        default:
            printf("unimplemented addr");
    }
    return 0;
}
void Cpu::exe_instruction(size_t opint, uint16_t addr)
{
    switch (opint) {
        case UNI: {
            printf("unimplemented instruction");
            return;
        } break;
        case ORA: {
            a |= mem->get(addr);
            set_zero_and_ng(a);
        } break;
        case AND: {
            a &= mem->get(addr);
            set_zero_and_ng(a);
        } break;
        case EOR: {
            a ^= mem->get(addr);
            set_zero_and_ng(a);
        } break;
        case ADC: {
            uint16_t value  = mem->get(addr);
            uint16_t result = a + value + (carry ? 1 : 0);
            carry           = result > 0xff;
            overflow        = (a & 0x80) == (value & 0x80) && (value & 0x80) != (result & 0x80);
            a               = result;
            set_zero_and_ng(a);
        } break;
        case SBC: {
            uint16_t value  = mem->get(addr) ^ 0xff;
            uint16_t result = a + value + (carry ? 1 : 0);
            carry           = result > 0xff;
            overflow        = (a & 0x80) == (value & 0x80) && (value & 0x80) != (result & 0x80);
            a               = result;
            set_zero_and_ng(a);
        } break;
        case CMP: {
            uint16_t value  = mem->get(addr) ^ 0xff;
            uint16_t result = a + value + 1;
            carry           = result > 0xff;
            set_zero_and_ng(result);
        } break;
        case CPX: {
            uint16_t value  = mem->get(addr) ^ 0xff;
            uint16_t result = x + value + 1;
            carry           = result > 0xff;
            set_zero_and_ng(result);
        } break;
        case CPY: {
            uint16_t value  = mem->get(addr) ^ 0xff;
            uint16_t result = y + value + 1;
            carry           = result > 0xff;
            set_zero_and_ng(result);
        } break;
        case DEC: {
            uint16_t data = mem->get(addr);
            if (data == 0) {
                data = 0xff;
            } else {
                data -= 1;
            }

            uint16_t result = data & 0xff;
            set_zero_and_ng(result);
            mem->set(addr, result);
        } break;
        case DEX: {
            if (x == 0) {
                x = 0xff;
            } else {
                x -= 1;
            }
            set_zero_and_ng(x);
        } break;
        case DEY: {
            if (y == 0) {
                y = 0xff;
            } else {
                y -= 1;
            }
            set_zero_and_ng(y);
        } break;
        case INC: {
            uint16_t result = ((mem->get(addr)) + 1) & 0xff;
            set_zero_and_ng(result);
            mem->set(addr, result);
        } break;
        case INX: {
            if (x == 255) {
                x = 0;
            } else {
                x += 1;
            }
            set_zero_and_ng(x);
        } break;
        case INY: {
            if (y == 255) {
                y = 0;
            } else {
                y += 1;
            }
            set_zero_and_ng(y);
        } break;
        case ASLA: {
            uint16_t result = (a) << 1;
            carry           = result > 0xff;
            set_zero_and_ng(result);
            a = result;
        } break;
        case ASL: {
            uint16_t result = (mem->get(addr)) << 1;
            carry           = result > 0xff;
            set_zero_and_ng(result);
            mem->set(addr, result);
        } break;
        case ROLA: {
            uint16_t result = ((a) << 1) | (carry ? 1 : 0);
            carry           = result > 0xff;
            set_zero_and_ng(result);
            a = result;
        } break;
        case ROL: {
            uint16_t result = ((mem->get(addr)) << 1) | (carry ? 1 : 0);
            carry           = result > 0xff;
            set_zero_and_ng(result);
            mem->set(addr, result);
        } break;
        case LSRA: {
            uint16_t _carry = a & 0x1;
            uint16_t result = a >> 1;
            carry           = _carry > 0;
            set_zero_and_ng(result);
            a = result;
        } break;
        case LSR: {
            uint16_t value  = mem->get(addr);
            uint16_t _carry = value & 0x1;
            uint16_t result = value >> 1;
            carry           = _carry > 0;
            set_zero_and_ng(result);
            mem->set(addr, result);
        } break;
        case RORA: {
            uint16_t _carry = a & 0x1;
            uint16_t result = (a >> 1) | ((carry ? 1 : 0) << 7);
            carry           = _carry > 0;
            set_zero_and_ng(result);
            a = result;
        } break;
        case ROR: {
            uint16_t value  = mem->get(addr);
            uint16_t _carry = value & 0x1;
            uint16_t result = (value >> 1) | ((carry ? 1 : 0) << 7);
            carry           = _carry > 0;
            set_zero_and_ng(result);
            mem->set(addr, result);
        } break;
        case LDA: {
            a = mem->get(addr);
            set_zero_and_ng(a);
        } break;
        case STA: {
            mem->set(addr, a);
        } break;
        case LDX: {
            x = mem->get(addr);
            set_zero_and_ng(x);
        } break;
        case STX: {
            mem->set(addr, x);
        } break;
        case LDY: {
            uint16_t _y = mem->get(addr);
            y           = _y;
            set_zero_and_ng(_y);
        } break;
        case STY: {
            mem->set(addr, y);
        } break;
        case TAX: {
            x = a;
            set_zero_and_ng(x);
        } break;
        case TXA: {
            a = x;
            set_zero_and_ng(a);
        } break;
        case TAY: {
            y = a;
            set_zero_and_ng(y);
        } break;
        case TYA: {
            a = y;
            set_zero_and_ng(a);
        } break;
        case TSX: {
            x = sp;
            set_zero_and_ng(x);
        } break;
        case TXS: {
            sp = x;
        } break;
        case PLA: {
            ++sp;
            uint16_t adr = 0x100 + (sp & 0xff);
            a            = mem->get(adr);
            set_zero_and_ng(a);
        } break;
        case PHA: {
            uint16_t adr = 0x100 + (sp-- & 0xff);
            mem->set(adr, a);
        } break;
        case PLP: {
            uint16_t adr = 0x100 + (++sp & 0xff);
            uint16_t val = mem->get(adr);
            setp(val);
        } break;
        case PHP: {
            uint16_t adr  = 0x100 + (sp-- & 0xff);
            uint16_t data = getp(true);
            mem->set(adr, data);
        } break;
        case BPL: {
            doBranch(!negative, addr);
        } break;
        case BMI: {
            doBranch(negative, addr);
        } break;
        case BVC: {
            doBranch(!overflow, addr);
        } break;
        case BVS: {
            doBranch(overflow, addr);
        } break;
        case BCC: {
            doBranch(!carry, addr);
        } break;
        case BCS: {
            doBranch(carry, addr);
        } break;
        case BNE: {
            doBranch(!zero, addr);
        } break;
        case BEQ: {
            doBranch(zero, addr);
        } break;
        case BRK: {
            uint16_t pushpc = (pc + 1) & 0xffff;

            uint16_t adr = 0x100 + sp--;
            mem->set(adr, (pushpc >> 8));

            uint16_t adr2 = 0x100 + sp--;
            mem->set(adr2, (pushpc & 0xff));

            uint16_t adr3 = 0x100 + sp--;
            uint16_t data = getp(true);
            mem->set(adr3, data);

            interrupt = true;
            pc        = mem->get(0xfffe) | ((mem->get(0xffff)) << 8);
        } break;
        case RTI: {
            ++sp;
            uint16_t adr = 0x100 + (sp & 0xff);
            uint16_t val = mem->get(adr);
            setp(val);

            ++sp;
            uint16_t adr2 = 0x100 + (sp & 0xff);
            uint16_t data = mem->get(adr2);

            ++sp;
            uint16_t adr3 = 0x100 + (sp & 0xff);
            uint16_t val2 = mem->get(adr3);
            uint16_t tmp  = val2 << 8;
            data |= tmp;
            pc = data;
        } break;
        case JSR: {
            uint16_t pushpc = (pc - 1) & 0xffff;

            uint16_t adr  = 0x100 + sp--;
            uint16_t data = pushpc >> 8;
            mem->set(adr, data);

            uint16_t adr2  = 0x100 + sp--;
            uint16_t data2 = pushpc & 0xff;
            mem->set(adr2, data2);
            pc = addr;
        } break;
        case RTS: {
            ++sp;
            uint16_t adr    = 0x100 + (sp & 0xff);
            uint16_t pullPc = mem->get(adr);

            ++sp;
            uint16_t adr2 = 0x100 + (sp & 0xff);
            uint16_t data = mem->get(adr2);
            uint16_t pp   = pullPc | (data << 8);
            pc            = pp + 1;
        } break;
        case JMP: {
            pc = addr;
        } break;
        case BIT: {
            uint16_t value = mem->get(addr);
            negative       = (value & 0x80) > 0;
            overflow       = (value & 0x40) > 0;
            uint16_t res   = a & value;
            zero           = res == 0;
        } break;
        case CLC: {
            carry = false;
        } break;
        case SEC: {
            carry = true;
        } break;
        case CLD: {
            decimal = false;
        } break;
        case SED: {
            decimal = true;
        } break;
        case CLI: {
            interrupt = false;
        } break;
        case SEI: {
            interrupt = true;
        } break;
        case CLV: {
            overflow = false;
        } break;
        case NOP: {
        } break;
        case IRQ: {
            uint16_t pushpc = pc;

            uint16_t adr  = 0x100 + sp++;
            uint16_t data = pushpc >> 8;
            mem->set(adr, data);

            uint16_t adr2  = 0x100 + sp++;
            uint16_t data2 = pushpc & 0xff;
            mem->set(adr2, data2);

            uint16_t adr3  = 0x100 + sp++;
            uint16_t data3 = getp(false);
            mem->set(adr3, data3);

            interrupt = true;
            pc        = mem->get(0xfffe) | ((mem->get(0xffff)) << 8);
        } break;
        case NMI: {
            uint16_t pushpc = pc;

            uint16_t adr  = 0x100 + sp--;
            uint16_t data = pushpc >> 8;
            mem->set(adr, data);

            uint16_t adr2  = 0x100 + sp--;
            uint16_t data2 = pushpc & 0xff;
            mem->set(adr2, data2);

            uint16_t adr3  = 0x100 + sp--;
            uint16_t data3 = getp(false);
            mem->set(adr3, data3);

            interrupt = true;
            pc        = mem->get(0xfffa) | ((mem->get(0xfffb)) << 8);
        } break;
        // undocumented opcodes
        case KIL: {
            pc--;
        } break;
        case SLO: {
            uint16_t data   = mem->get(addr);
            uint16_t result = data << 1;

            carry = result > 0xff;
            mem->set(addr, result);
            a |= result;
            set_zero_and_ng(a);
        } break;
        case RLA: {
            uint16_t data   = mem->get(addr);
            uint16_t result = (data << 1) | (carry ? 1 : 0);
            carry           = result > 0xff;
            mem->set(addr, result);
            a &= result;
            set_zero_and_ng(a);
        } break;
        case SRE: {
            uint16_t value  = mem->get(addr);
            uint16_t _carry = value & 0x1;
            uint16_t result = value >> 1;
            carry           = _carry > 0;
            mem->set(addr, result);
            a ^= result;
            set_zero_and_ng(a);
        } break;
        case RRA: {
            uint16_t value  = mem->get(addr);
            uint16_t _carry = value & 0x1;
            uint16_t result = (value >> 1) | ((carry ? 1 : 0) << 7);
            mem->set(addr, result);
            uint16_t data = a + result + _carry;
            carry         = data > 0xff;
            overflow      = (a & 0x80) == (result & 0x80) && (result & 0x80) != (data & 0x80);
            a             = data;
            set_zero_and_ng(a);
        } break;
        case SAX: {
            mem->set(addr, a & x);
        } break;
        case LAX: {
            a = mem->get(addr);
            x = a;
            set_zero_and_ng(x);
        } break;
        case DCP: {
            uint16_t dat = mem->get(addr);
            if (dat == 0) {
                dat = 0xff;
            } else {
                dat -= 1;
            }
            uint16_t value = dat & 0xff;
            mem->set(addr, value);
            value ^= 0xff;
            uint16_t result = a + value + 1;
            carry           = result > 0xff;
            set_zero_and_ng(result & 0xff);
        } break;
        case ISC: {
            uint16_t dat = mem->get(addr);
            if (dat == 0xff) {
                dat = 0;
            } else {
                dat += 1;
            }
            uint16_t value = dat & 0xff;
            mem->set(addr, value);
            value ^= 0xff;

            uint16_t result = a + value + (carry ? 1 : 0);

            carry    = result > 0xff;
            overflow = (a & 0x80) == (value & 0x80) && (value & 0x80) != ((result & 0x80));

            a = result;
            set_zero_and_ng(a);
        } break;
        case ANC: {
            a &= mem->get(addr);
            set_zero_and_ng(a);
            carry = negative;
        } break;
        case ALR: {
            a &= mem->get(addr);
            uint16_t _carry = a & 0x1;
            uint16_t result = a >> 1;
            carry           = _carry > 0;
            set_zero_and_ng(result);
            a = result;
        } break;
        case ARR: {
            a &= mem->get(addr);
            uint16_t result = (a >> 1) | ((carry ? 1 : 0) << 7);
            set_zero_and_ng(result);
            carry    = (result & 0x40) > 0;
            overflow = ((result & 0x40) ^ ((result & 0x20) << 1)) > 0;
            a        = result;
        } break;
        case AXS: {
            uint16_t value  = mem->get(addr) ^ 0xff;
            uint16_t data   = a & x;
            uint16_t result = data + value + 1;
            carry           = result > 0xff;
            x               = result;
            set_zero_and_ng(x);
        } break;
        default:
            printf("unimplemented opcode");
            exit(1);
    }
}
void Cpu::set_zero_and_ng(uint8_t rval)
{
    int val  = rval & 0xff;
    zero     = val == 0;
    negative = val > 0x7f;
}
void Cpu::setp(uint8_t value)
{
    negative  = (value & 0x80) > 0;
    overflow  = (value & 0x40) > 0;
    decimal   = (value & 0x08) > 0;
    interrupt = (value & 0x04) > 0;
    zero      = (value & 0x02) > 0;
    carry     = (value & 0x01) > 0;
}
uint8_t Cpu::getp(bool bFlag)
{
    uint8_t value = 0;
    value |= negative ? 0x80 : 0;
    value |= overflow ? 0x40 : 0;
    value |= decimal ? 0x08 : 0;
    value |= interrupt ? 0x04 : 0;
    value |= zero ? 0x02 : 0;
    value |= carry ? 0x01 : 0;
    value |= 0x20;
    value |= bFlag ? 0x10 : 0;
    return value;
}
void Cpu::doBranch(bool test, uint16_t reladr)
{
    if (test) {
        cpuclock += 1;

        uint16_t u8val  = reladr;
        uint32_t relval = 0;
        uint16_t adr    = 0;

        if (u8val > 127) {
            relval = (256 - u8val);
            adr    = (pc - relval);
        } else {
            adr = (pc + u8val);
        }

        if (pc >> 8 != adr >> 8) {
            cpuclock += 1;
        }
        pc = adr;
    }
}
void Cpu::create_opcode(size_t opcode, size_t opint, string hex, string op, size_t adm, size_t cycle)
{
    opcodes[opint] = Opcode{.opcode = opcode, .opint = opint, .hex = hex, .op = op, .adm = adm, .cycle = cycle};
}
void Cpu::create_opcodes()
{
    create_opcode(BRK, 0, "0", "BRK", IMP, 7);
    create_opcode(ORA, 1, "1", "ORA", IZX, 6);
    create_opcode(KIL, 2, "2", "KIL", IMP, 2);
    create_opcode(SLO, 3, "3", "SLO", IZX, 8);
    create_opcode(NOP, 4, "4", "NOP", ZP, 3);
    create_opcode(ORA, 5, "5", "ORA", ZP, 3);
    create_opcode(ASL, 6, "6", "ASL", ZP, 5);
    create_opcode(SLO, 7, "7", "SLO", ZP, 5);
    create_opcode(PHP, 8, "8", "PHP", IMP, 3);
    create_opcode(ORA, 9, "9", "ORA", IMM, 2);
    create_opcode(ASLA, 10, "a", "ASLA", IMP, 2);
    create_opcode(ANC, 11, "b", "ANC", IMM, 2);
    create_opcode(NOP, 12, "c", "NOP", ABS, 4);
    create_opcode(ORA, 13, "d", "ORA", ABS, 4);
    create_opcode(ASL, 14, "e", "ASL", ABS, 6);
    create_opcode(SLO, 15, "f", "SLO", ABS, 6);
    create_opcode(BPL, 16, "10", "BPL", REL, 2);
    create_opcode(ORA, 17, "11", "ORA", IZYr, 5);
    create_opcode(KIL, 18, "12", "KIL", IMP, 2);
    create_opcode(SLO, 19, "13", "SLO", IZY, 8);
    create_opcode(NOP, 20, "14", "NOP", ZPX, 4);
    create_opcode(ORA, 21, "15", "ORA", ZPX, 4);
    create_opcode(ASL, 22, "16", "ASL", ZPX, 6);
    create_opcode(SLO, 23, "17", "SLO", ZPX, 6);
    create_opcode(CLC, 24, "18", "CLC", IMP, 2);
    create_opcode(ORA, 25, "19", "ORA", ABYr, 4);
    create_opcode(NOP, 26, "1a", "NOP", IMP, 2);
    create_opcode(SLO, 27, "1b", "SLO", ABY, 7);
    create_opcode(NOP, 28, "1c", "NOP", ABXr, 4);
    create_opcode(ORA, 29, "1d", "ORA", ABXr, 4);
    create_opcode(ASL, 30, "1e", "ASL", ABX, 7);
    create_opcode(SLO, 31, "1f", "SLO", ABX, 7);
    create_opcode(JSR, 32, "20", "JSR", ABS, 6);
    create_opcode(AND, 33, "21", "AND", IZX, 6);
    create_opcode(KIL, 34, "22", "KIL", IMP, 2);
    create_opcode(RLA, 35, "23", "RLA", IZX, 8);
    create_opcode(BIT, 36, "24", "BIT", ZP, 3);
    create_opcode(AND, 37, "25", "AND", ZP, 3);
    create_opcode(ROL, 38, "26", "ROL", ZP, 5);
    create_opcode(RLA, 39, "27", "RLA", ZP, 5);
    create_opcode(PLP, 40, "28", "PLP", IMP, 4);
    create_opcode(AND, 41, "29", "AND", IMM, 2);
    create_opcode(ROLA, 42, "2a", "ROLA", IMP, 2);
    create_opcode(ANC, 43, "2b", "ANC", IMM, 2);
    create_opcode(BIT, 44, "2c", "BIT", ABS, 4);
    create_opcode(AND, 45, "2d", "AND", ABS, 4);
    create_opcode(ROL, 46, "2e", "ROL", ABS, 6);
    create_opcode(RLA, 47, "2f", "RLA", ABS, 6);
    create_opcode(BMI, 48, "30", "BMI", REL, 2);
    create_opcode(AND, 49, "31", "AND", IZYr, 5);
    create_opcode(KIL, 50, "32", "KIL", IMP, 2);
    create_opcode(RLA, 51, "33", "RLA", IZY, 8);
    create_opcode(NOP, 52, "34", "NOP", ZPX, 4);
    create_opcode(AND, 53, "35", "AND", ZPX, 4);
    create_opcode(ROL, 54, "36", "ROL", ZPX, 6);
    create_opcode(RLA, 55, "37", "RLA", ZPX, 6);
    create_opcode(SEC, 56, "38", "SEC", IMP, 2);
    create_opcode(AND, 57, "39", "AND", ABYr, 4);
    create_opcode(NOP, 58, "3a", "NOP", IMP, 2);
    create_opcode(RLA, 59, "3b", "RLA", ABY, 7);
    create_opcode(NOP, 60, "3c", "NOP", ABXr, 4);
    create_opcode(AND, 61, "3d", "AND", ABXr, 4);
    create_opcode(ROL, 62, "3e", "ROL", ABX, 7);
    create_opcode(RLA, 63, "3f", "RLA", ABX, 7);
    create_opcode(RTI, 64, "40", "RTI", IMP, 6);
    create_opcode(EOR, 65, "41", "EOR", IZX, 6);
    create_opcode(KIL, 66, "42", "KIL", IMP, 2);
    create_opcode(SRE, 67, "43", "SRE", IZX, 8);
    create_opcode(NOP, 68, "44", "NOP", ZP, 3);
    create_opcode(EOR, 69, "45", "EOR", ZP, 3);
    create_opcode(LSR, 70, "46", "LSR", ZP, 5);
    create_opcode(SRE, 71, "47", "SRE", ZP, 5);
    create_opcode(PHA, 72, "48", "PHA", IMP, 3);
    create_opcode(EOR, 73, "49", "EOR", IMM, 2);
    create_opcode(LSRA, 74, "4a", "LSRA", IMP, 2);
    create_opcode(ALR, 75, "4b", "ALR", IMM, 2);
    create_opcode(JMP, 76, "4c", "JMP", ABS, 3);
    create_opcode(EOR, 77, "4d", "EOR", ABS, 4);
    create_opcode(LSR, 78, "4e", "LSR", ABS, 6);
    create_opcode(SRE, 79, "4f", "SRE", ABS, 6);
    create_opcode(BVC, 80, "50", "BVC", REL, 2);
    create_opcode(EOR, 81, "51", "EOR", IZYr, 5);
    create_opcode(KIL, 82, "52", "KIL", IMP, 2);
    create_opcode(SRE, 83, "53", "SRE", IZY, 8);
    create_opcode(NOP, 84, "54", "NOP", ZPX, 4);
    create_opcode(EOR, 85, "55", "EOR", ZPX, 4);
    create_opcode(LSR, 86, "56", "LSR", ZPX, 6);
    create_opcode(SRE, 87, "57", "SRE", ZPX, 6);
    create_opcode(CLI, 88, "58", "CLI", IMP, 2);
    create_opcode(EOR, 89, "59", "EOR", ABYr, 4);
    create_opcode(NOP, 90, "5a", "NOP", IMP, 2);
    create_opcode(SRE, 91, "5b", "SRE", ABY, 7);
    create_opcode(NOP, 92, "5c", "NOP", ABXr, 4);
    create_opcode(EOR, 93, "5d", "EOR", ABXr, 4);
    create_opcode(LSR, 94, "5e", "LSR", ABX, 7);
    create_opcode(SRE, 95, "5f", "SRE", ABX, 7);
    create_opcode(RTS, 96, "60", "RTS", IMP, 6);
    create_opcode(ADC, 97, "61", "ADC", IZX, 6);
    create_opcode(KIL, 98, "62", "KIL", IMP, 2);
    create_opcode(RRA, 99, "63", "RRA", IZX, 8);
    create_opcode(NOP, 100, "64", "NOP", ZP, 3);
    create_opcode(ADC, 101, "65", "ADC", ZP, 3);
    create_opcode(ROR, 102, "66", "ROR", ZP, 5);
    create_opcode(RRA, 103, "67", "RRA", ZP, 5);
    create_opcode(PLA, 104, "68", "PLA", IMP, 4);
    create_opcode(ADC, 105, "69", "ADC", IMM, 2);
    create_opcode(RORA, 106, "6a", "RORA", IMP, 2);
    create_opcode(ARR, 107, "6b", "ARR", IMM, 2);
    create_opcode(JMP, 108, "6c", "JMP", IND, 5);
    create_opcode(ADC, 109, "6d", "ADC", ABS, 4);
    create_opcode(ROR, 110, "6e", "ROR", ABS, 6);
    create_opcode(RRA, 111, "6f", "RRA", ABS, 6);
    create_opcode(BVS, 112, "70", "BVS", REL, 2);
    create_opcode(ADC, 113, "71", "ADC", IZYr, 5);
    create_opcode(KIL, 114, "72", "KIL", IMP, 2);
    create_opcode(RRA, 115, "73", "RRA", IZY, 8);
    create_opcode(NOP, 116, "74", "NOP", ZPX, 4);
    create_opcode(ADC, 117, "75", "ADC", ZPX, 4);
    create_opcode(ROR, 118, "76", "ROR", ZPX, 6);
    create_opcode(RRA, 119, "77", "RRA", ZPX, 6);
    create_opcode(SEI, 120, "78", "SEI", IMP, 2);
    create_opcode(ADC, 121, "79", "ADC", ABYr, 4);
    create_opcode(NOP, 122, "7a", "NOP", IMP, 2);
    create_opcode(RRA, 123, "7b", "RRA", ABY, 7);
    create_opcode(NOP, 124, "7c", "NOP", ABXr, 4);
    create_opcode(ADC, 125, "7d", "ADC", ABXr, 4);
    create_opcode(ROR, 126, "7e", "ROR", ABX, 7);
    create_opcode(RRA, 127, "7f", "RRA", ABX, 7);
    create_opcode(NOP, 128, "80", "NOP", IMM, 2);
    create_opcode(STA, 129, "81", "STA", IZX, 6);
    create_opcode(NOP, 130, "82", "NOP", IMM, 2);
    create_opcode(SAX, 131, "83", "SAX", IZX, 6);
    create_opcode(STY, 132, "84", "STY", ZP, 3);
    create_opcode(STA, 133, "85", "STA", ZP, 3);
    create_opcode(STX, 134, "86", "STX", ZP, 3);
    create_opcode(SAX, 135, "87", "SAX", ZP, 3);
    create_opcode(DEY, 136, "88", "DEY", IMP, 2);
    create_opcode(NOP, 137, "89", "NOP", IMM, 2);
    create_opcode(TXA, 138, "8a", "TXA", IMP, 2);
    create_opcode(UNI, 139, "8b", "UNI", IMM, 2);
    create_opcode(STY, 140, "8c", "STY", ABS, 4);
    create_opcode(STA, 141, "8d", "STA", ABS, 4);
    create_opcode(STX, 142, "8e", "STX", ABS, 4);
    create_opcode(SAX, 143, "8f", "SAX", ABS, 4);
    create_opcode(BCC, 144, "90", "BCC", REL, 2);
    create_opcode(STA, 145, "91", "STA", IZY, 6);
    create_opcode(KIL, 146, "92", "KIL", IMP, 2);
    create_opcode(UNI, 147, "93", "UNI", IZY, 6);
    create_opcode(STY, 148, "94", "STY", ZPX, 4);
    create_opcode(STA, 149, "95", "STA", ZPX, 4);
    create_opcode(STX, 150, "96", "STX", ZPY, 4);
    create_opcode(SAX, 151, "97", "SAX", ZPY, 4);
    create_opcode(TYA, 152, "98", "TYA", IMP, 2);
    create_opcode(STA, 153, "99", "STA", ABY, 5);
    create_opcode(TXS, 154, "9a", "TXS", IMP, 2);
    create_opcode(UNI, 155, "9b", "UNI", ABY, 5);
    create_opcode(UNI, 156, "9c", "UNI", ABX, 5);
    create_opcode(STA, 157, "9d", "STA", ABX, 5);
    create_opcode(UNI, 158, "9e", "UNI", ABY, 5);
    create_opcode(UNI, 159, "9f", "UNI", ABY, 5);
    create_opcode(LDY, 160, "a0", "LDY", IMM, 2);
    create_opcode(LDA, 161, "a1", "LDA", IZX, 6);
    create_opcode(LDX, 162, "a2", "LDX", IMM, 2);
    create_opcode(LAX, 163, "a3", "LAX", IZX, 6);
    create_opcode(LDY, 164, "a4", "LDY", ZP, 3);
    create_opcode(LDA, 165, "a5", "LDA", ZP, 3);
    create_opcode(LDX, 166, "a6", "LDX", ZP, 3);
    create_opcode(LAX, 167, "a7", "LAX", ZP, 3);
    create_opcode(TAY, 168, "a8", "TAY", IMP, 2);
    create_opcode(LDA, 169, "a9", "LDA", IMM, 2);
    create_opcode(TAX, 170, "aa", "TAX", IMP, 2);
    create_opcode(UNI, 171, "ab", "UNI", IMM, 2);
    create_opcode(LDY, 172, "ac", "LDY", ABS, 4);
    create_opcode(LDA, 173, "ad", "LDA", ABS, 4);
    create_opcode(LDX, 174, "ae", "LDX", ABS, 4);
    create_opcode(LAX, 175, "af", "LAX", ABS, 4);
    create_opcode(BCS, 176, "b0", "BCS", REL, 2);
    create_opcode(LDA, 177, "b1", "LDA", IZYr, 5);
    create_opcode(KIL, 178, "b2", "KIL", IMP, 2);
    create_opcode(LAX, 179, "b3", "LAX", IZYr, 5);
    create_opcode(LDY, 180, "b4", "LDY", ZPX, 4);
    create_opcode(LDA, 181, "b5", "LDA", ZPX, 4);
    create_opcode(LDX, 182, "b6", "LDX", ZPY, 4);
    create_opcode(LAX, 183, "b7", "LAX", ZPY, 4);
    create_opcode(CLV, 184, "b8", "CLV", IMP, 2);
    create_opcode(LDA, 185, "b9", "LDA", ABYr, 4);
    create_opcode(TSX, 186, "ba", "TSX", IMP, 2);
    create_opcode(UNI, 187, "bb", "UNI", ABYr, 4);
    create_opcode(LDY, 188, "bc", "LDY", ABXr, 4);
    create_opcode(LDA, 189, "bd", "LDA", ABXr, 4);
    create_opcode(LDX, 190, "be", "LDX", ABYr, 4);
    create_opcode(LAX, 191, "bf", "LAX", ABYr, 4);
    create_opcode(CPY, 192, "c0", "CPY", IMM, 2);
    create_opcode(CMP, 193, "c1", "CMP", IZX, 6);
    create_opcode(NOP, 194, "c2", "NOP", IMM, 2);
    create_opcode(DCP, 195, "c3", "DCP", IZX, 8);
    create_opcode(CPY, 196, "c4", "CPY", ZP, 3);
    create_opcode(CMP, 197, "c5", "CMP", ZP, 3);
    create_opcode(DEC, 198, "c6", "DEC", ZP, 5);
    create_opcode(DCP, 199, "c7", "DCP", ZP, 5);
    create_opcode(INY, 200, "c8", "INY", IMP, 2);
    create_opcode(CMP, 201, "c9", "CMP", IMM, 2);
    create_opcode(DEX, 202, "ca", "DEX", IMP, 2);
    create_opcode(AXS, 203, "cb", "AXS", IMM, 2);
    create_opcode(CPY, 204, "cc", "CPY", ABS, 4);
    create_opcode(CMP, 205, "cd", "CMP", ABS, 4);
    create_opcode(DEC, 206, "ce", "DEC", ABS, 6);
    create_opcode(DCP, 207, "cf", "DCP", ABS, 6);
    create_opcode(BNE, 208, "d0", "BNE", REL, 2);
    create_opcode(CMP, 209, "d1", "CMP", IZYr, 5);
    create_opcode(KIL, 210, "d2", "KIL", IMP, 2);
    create_opcode(DCP, 211, "d3", "DCP", IZY, 8);
    create_opcode(NOP, 212, "d4", "NOP", ZPX, 4);
    create_opcode(CMP, 213, "d5", "CMP", ZPX, 4);
    create_opcode(DEC, 214, "d6", "DEC", ZPX, 6);
    create_opcode(DCP, 215, "d7", "DCP", ZPX, 6);
    create_opcode(CLD, 216, "d8", "CLD", IMP, 2);
    create_opcode(CMP, 217, "d9", "CMP", ABYr, 4);
    create_opcode(NOP, 218, "da", "NOP", IMP, 2);
    create_opcode(DCP, 219, "db", "DCP", ABY, 7);
    create_opcode(NOP, 220, "dc", "NOP", ABXr, 4);
    create_opcode(CMP, 221, "dd", "CMP", ABXr, 4);
    create_opcode(DEC, 222, "de", "DEC", ABX, 7);
    create_opcode(DCP, 223, "df", "DCP", ABX, 7);
    create_opcode(CPX, 224, "e0", "CPX", IMM, 2);
    create_opcode(SBC, 225, "e1", "SBC", IZX, 6);
    create_opcode(NOP, 226, "e2", "NOP", IMM, 2);
    create_opcode(ISC, 227, "e3", "ISC", IZX, 8);
    create_opcode(CPX, 228, "e4", "CPX", ZP, 3);
    create_opcode(SBC, 229, "e5", "SBC", ZP, 3);
    create_opcode(INC, 230, "e6", "INC", ZP, 5);
    create_opcode(ISC, 231, "e7", "ISC", ZP, 5);
    create_opcode(INX, 232, "e8", "INX", IMP, 2);
    create_opcode(SBC, 233, "e9", "SBC", IMM, 2);
    create_opcode(NOP, 234, "ea", "NOP", IMP, 2);
    create_opcode(SBC, 235, "eb", "SBC", IMM, 2);
    create_opcode(CPX, 236, "ec", "CPX", ABS, 4);
    create_opcode(SBC, 237, "ed", "SBC", ABS, 4);
    create_opcode(INC, 238, "ee", "INC", ABS, 6);
    create_opcode(ISC, 239, "ef", "ISC", ABS, 6);
    create_opcode(BEQ, 240, "f0", "BEQ", REL, 2);
    create_opcode(SBC, 241, "f1", "SBC", IZYr, 5);
    create_opcode(KIL, 242, "f2", "KIL", IMP, 2);
    create_opcode(ISC, 243, "f3", "ISC", IZY, 8);
    create_opcode(NOP, 244, "f4", "NOP", ZPX, 4);
    create_opcode(SBC, 245, "f5", "SBC", ZPX, 4);
    create_opcode(INC, 246, "f6", "INC", ZPX, 6);
    create_opcode(ISC, 247, "f7", "ISC", ZPX, 6);
    create_opcode(SED, 248, "f8", "SED", IMP, 2);
    create_opcode(SBC, 249, "f9", "SBC", ABYr, 4);
    create_opcode(NOP, 250, "fa", "NOP", IMP, 2);
    create_opcode(ISC, 251, "fb", "ISC", ABY, 7);
    create_opcode(NOP, 252, "fc", "NOP", ABXr, 4);
    create_opcode(SBC, 253, "fd", "SBC", ABXr, 4);
    create_opcode(INC, 254, "fe", "INC", ABX, 7);
    create_opcode(ISC, 255, "ff", "ISC", ABX, 7);
    create_opcode(NMI, 256, "100", "NMI", NMI, 0);
    create_opcode(IRQ, 257, "101", "IRQ", IRQ, 0);
};
