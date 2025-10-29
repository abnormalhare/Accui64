#include "../inc/debug.hpp"
#include "../inc/ram.hpp"
#include "../inc/x64.hpp"
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <variant>

CPU::CPU() {
    this->extra_info = std::unordered_map<const char *, u8>();
    this->extra_info.clear();

    this->running = true;

    this->setupRegs();
}

void CPU::setupRegs() {
    for (int i = 0; i < 0x10; i++) {
        this->regs[i] = 0;
    }
    this->IP->r = 0xFFF0;

    for (int i = 0; i < 6; i++) {
        this->st_regs[i].selector = 0;
        this->st_regs[i].limit = 0xFFFF;
        this->st_regs[i].base = 0;
        this->st_regs[i].attr = 0;
    }
    this->CS->selector = 0xF000;
    this->CS->base = 0xFFFF0000;

    RFLAGS._0 = 1;

    *CR0 = { 0 };
    CR0->et = 1;
    CR0->mp = 1;
    CR0->ne = 1;

    *CR2 = 0;
    *CR3 = { 0 };
    *CR4 = { 0 };

    *DR0 = 0;
    *DR1 = 0;
    *DR2 = 0;
    *DR3 = 0;
    *DR6 = 0xFFFF0FF0;
    
    *DR7 = { 0 };
    DR7->dr0_size = 1;

    LDTR = 0;
    TR = 0;
    GDTR.addr = 0;
    IDTR.addr = 0;

    for (int i = 0; i < 8; i++) {
        this->mm_regs[i] = 0;
    }
    for (int i = 0; i < 16; i++) {
        this->xm_regs[i].n256[0] = _mm256_setzero_si256();
        this->xm_regs[i].n256[1] = _mm256_setzero_si256();
    }

    IA32_EFER = { 0 };
}

void CPU::run() {
    this->extra_info.insert({"rex", 0x00});

    std::cout << "---------------------------" << std::endl;
    std::cout << "EIP: " << std::hex << std::uppercase << (int)(CS->base + IP->e) << std::endl << std::endl;
    while (this->running) {
        if (this->runStep()) continue;
        
        std::cout << std::endl;
        this->debugPrintRegs();
        std::cout << "---------------------------" << std::endl;
        std::cout << "EIP: " << std::hex << std::uppercase << (int)(CS->base + IP->e) << std::endl << std::endl;
    }
}

bool CPU::runStep() {
    this->curr_inst = this->read();

    bool dont_clear = (this->*CPU::opcode_table[this->curr_inst])();

    if (!dont_clear) {
        this->extra_info.clear();
        this->extra_info.insert({"rex", 0x00});
    }
    return dont_clear;
}

u8 CPU::read() {
    u8 ret = RAM::read(CS->base + IP->e);
    IP->x++;

    return ret;
}

void CPU::write(u64 addr, u8 val) {
    RAM::write(addr, val);
}

void CPU::writeReg(u64 addr, Reg *reg, RegType type) {
    auto val = reg->get(type);
    std::visit([&](auto write) {
        using T = decltype(write);

        for (int i = 0; i < sizeof(T); i++) {
            this->write(addr + i, write >> (i * 8));
        }
    }, val);
}

u8 CPU::getVal8() {
    return this->read();
}

u16 CPU::getVal16() {
    return this->read() + (this->read() << 8);
}

u32 CPU::getVal32() {
    u32 val = this->read();
    val |= static_cast<u32>(this->read()) << 8;
    val |= static_cast<u32>(this->read()) << 16;
    val |= static_cast<u32>(this->read()) << 24;
    return val;
}

bool CPU::checkExceptions(u64 ptr, const std::vector<ExceptionType> &exceptions) {
    // Check for basic protection faults
    if (CR0->pe) {  // Protected mode checks
        for (const auto &exception : exceptions) {
            switch (exception) {
                case ExceptionType::DE:  // Divide Error
                    // Handled during division operations
                    break;
                    
                case ExceptionType::DB:  // Debug Exception
                    // Check debug registers if any breakpoints are hit
                    for (int i = 0; i < 4; i++) {
                        if (DR7->get_enable(i) && ptr == this->db_regs[i]) return true;
                    }
                    break;
                    
                case ExceptionType::NMI:  // Non-Maskable Interrupt
                    // External hardware interrupt, not checked here
                    break;
                    
                case ExceptionType::BP:  // Breakpoint
                    // Software triggered (INT3), not checked here
                    break;
                    
                case ExceptionType::OF:  // Overflow
                    // Handled in arithmetic operations
                    break;
                    
                case ExceptionType::BR:  // BOUND Range exceeded
                    // Handled in BOUND instruction
                    break;
                    
                case ExceptionType::UD:  // Invalid Opcode
                    // Handled during instruction decode
                    break;
                    
                case ExceptionType::NM:  // Device Not Available
                    if (CR0->em || CR0->ts) return true;
                    break;
                    
                case ExceptionType::DF:  // Double Fault
                    // Complex exception handling, not implemented here
                    break;
                    
                case ExceptionType::CSO:  // Coprocessor Segment Overrun
                    // Legacy exception, not used in modern CPUs
                    break;
                    
                case ExceptionType::TS:  // Invalid TSS
                    // Task switching not implemented yet
                    break;
                    
                case ExceptionType::NP:  // Segment Not Present
                    // Check segment presence
                    if (!(CS->attr & 0x80)) return true;
                    break;
                    
                case ExceptionType::SS:  // Stack Segment Fault
                    // Check stack segment access
                    if ((ptr >= SS->base) && (ptr <= SS->base + SS->limit)) {
                        if (!(SS->attr & 0x2)) return true;  // Write protect check
                    }
                    break;
                    
                case ExceptionType::GP:  // General Protection Fault
                    // Check segment limits and privileges
                    if (ptr > CS->limit) return true;
                    break;
                    
                case ExceptionType::PF:  // Page Fault
                    if (CR0->pg) {  // Paging enabled
                        // Basic page presence check
                        // Note: Full paging implementation would need page table walk
                        if (ptr > 0xFFFFFFFF) return true;
                    }
                    break;
                    
                case ExceptionType::MF:  // x87 FPU Floating-Point Error
                    // FPU errors handled separately
                    break;
                    
                case ExceptionType::AC:  // Alignment Check
                    if (CR0->am && (RFLAGS.ac)) {
                        if (ptr & 0x3) return true;  // Check if address is aligned
                    }
                    break;
                    
                case ExceptionType::MC:  // Machine Check
                    // Hardware-level errors, not implemented
                    break;
                    
                case ExceptionType::XM:  // SIMD Floating-Point Exception
                    // SIMD errors handled separately
                    break;
                    
                case ExceptionType::VE:  // Virtualization Exception
                    // Virtualization events not implemented
                    break;
                    
                case ExceptionType::SX:  // Security Exception
                    // Security violations not implemented
                    break;
                    
                default:
                    break;
            }
        }
    }
    return false;
}

bool CPU::HALT() {
    this->running = false;
    return true;
}

void CPU::determineModRMMod3(ModRM *modrm, RegType type) {
    u8 rmidx  = ((this->extra_info["rex"] & REXBit::B) << 3) | modrm->_rm ;
    u8 regidx = ((this->extra_info["rex"] & REXBit::R) << 3) | modrm->_reg;
    
    switch (type) {
        case RegType::R8: case RegType::R8H:
            modrm->reg_type = modrm->rm_type  = RegType::R8;

            if (modrm->_reg >= 4 && modrm->_reg < 8) {
                modrm->_reg -= 4; modrm->reg_type = RegType::R8H;
            }
            modrm->rm  = &this->regs[rmidx ];
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::R16: case RegType::R32: case RegType::R64:
            modrm->reg_type = modrm->rm_type  = RegType::R32;

            if (this->extra_info.contains("op")) { // TODO order
                modrm->reg_type = modrm->rm_type  = RegType::R16;
            }
            if (this->extra_info["rex"] & REXBit::W) {
                modrm->reg_type = modrm->rm_type  = RegType::R64;
            }
            modrm->rm  = &this->regs[rmidx ];
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::ST:
            modrm->reg_type = modrm->rm_type  = RegType::ST;

            modrm->rm  = &this->st_regs[modrm->_rm ];
            modrm->reg = &this->st_regs[modrm->_reg];
            break;
        
        case RegType::MM:
            modrm->reg_type = modrm->rm_type  = RegType::MM;
            
            modrm->rm  = &this->mm_regs[modrm->_rm ];
            modrm->reg = &this->mm_regs[modrm->_reg];
            break;
        
        case RegType::XMM:
            modrm->reg_type = modrm->rm_type  = RegType::XMM;
            
            modrm->rm  = &this->xm_regs[rmidx ];
            modrm->reg = &this->xm_regs[regidx];
            break;
    }
}

void CPU::determineModRMMod0to2(ModRM *modrm, RegType type) {
    u8 rmidx  = ((this->extra_info["rex"] & REXBit::B) << 3) | modrm->_rm ;
    u8 regidx = ((this->extra_info["rex"] & REXBit::R) << 3) | modrm->_reg;

    std::cout << "MODRM: MOD " << (int)modrm->_mod << " | REG " << (int)regidx << " | RM " << (int)rmidx << std::endl;

    modrm->rm_type = RegType::R64;
    if (this->extra_info.contains("ad")) {
        modrm->rm_type = RegType::R32;
    }

    switch (type) {
        case RegType::R8: case RegType::R8H:
            modrm->reg_type = RegType::R8;

            if (modrm->_reg >= 4 && modrm->_reg < 8 && !this->extra_info.contains("rex")) {
                modrm->_reg -= 4; modrm->reg_type = RegType::R8H;
            }
            modrm->rm  = &this->regs[rmidx ];
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::R16: case RegType::R32: case RegType::R64:
            modrm->reg_type = RegType::R32;

            if (this->extra_info.contains("op")) { // TODO order
                modrm->reg_type = RegType::R16;
            }
            if (this->extra_info["rex"] & REXBit::W) {
                modrm->reg_type = RegType::R64;
            }
            modrm->rm  = &this->regs[rmidx ];
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::ST:
            modrm->reg_type = RegType::ST;

            modrm->rm  = &this->st_regs[rmidx  & 7];
            modrm->reg = &this->st_regs[regidx & 7];
            break;
        
        case RegType::MM:
            modrm->reg_type = RegType::MM;
            
            modrm->rm  = &this->mm_regs[rmidx  & 7];
            modrm->reg = &this->mm_regs[regidx & 7];
            break;
        
        case RegType::XMM:
            modrm->reg_type = RegType::XMM;
            
            modrm->rm  = &this->xm_regs[rmidx ];
            modrm->reg = &this->xm_regs[regidx];
            break;
    }
}

void CPU::determineModRMSib(ModRM *modrm, RegType type, u8 sib) {
    modrm->sib._ss   = getMask(sib, 7, 6);
    modrm->sib._base = getMask(sib, 5, 3);
    modrm->sib._idx  = getMask(sib, 2, 0);
    
    u8 idxidx  = ((this->extra_info["rex"] & REXBit::X) << 3) | modrm->sib._idx;
    u8 baseidx = ((this->extra_info["rex"] & REXBit::B) << 3) | modrm->sib._base;
    u8 regidx = ((this->extra_info["rex"] & REXBit::R)  << 3) | modrm->_reg;

    modrm->sib.idx  = (idxidx == 4) ? nullptr : &this->regs[idxidx];
    modrm->sib.base = &this->regs[baseidx];
    modrm->sib.mul  = (modrm->sib._ss != 0) ? modrm->sib._ss * 2 : 1;

    if (modrm->sib._base == 5 && modrm->_mod == 0) {
        modrm->disp = 4;
        modrm->sib.base = nullptr;
    }
    

    modrm->sib.idx_type  = RegType::R64;
    modrm->sib.base_type = RegType::R64;
    if (this->extra_info.contains("ad")) {
        modrm->sib.idx_type  = RegType::R32;
        modrm->sib.base_type = RegType::R32;
    }

    switch (type) {
        case RegType::R8: case RegType::R8H:
            modrm->reg_type = RegType::R8;

            if (modrm->_reg >= 4 && modrm->_reg < 8 && this->extra_info["rex"] == 0x00) {
                modrm->_reg -= 4; modrm->reg_type = RegType::R8H;
            }
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::R16: case RegType::R32: case RegType::R64:
            modrm->reg_type = RegType::R32;

            if (this->extra_info.contains("op")) { // TODO order
                modrm->reg_type = RegType::R16;
            }
            if (this->extra_info["rex"] & REXBit::W) {
                modrm->reg_type = RegType::R64;
            }
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::ST:
            modrm->reg_type = RegType::ST;
            modrm->reg = &this->st_regs[modrm->_reg];
            break;
        
        case RegType::MM:
            modrm->reg_type = RegType::MM;
            modrm->reg = &this->mm_regs[modrm->_reg];
            break;
        
        case RegType::XMM:
            modrm->reg_type = RegType::XMM;
            modrm->reg = &this->xm_regs[regidx];
            break;
    }
}

ModRM *CPU::getModRM16(RegType type) {
    u8 val = this->read();
    ModRM *modrm = new ModRM(
        getMask(val, 7, 6),
        getMask(val, 5, 3),
        getMask(val, 2, 0)
    );

    modrm->rm = nullptr;
    modrm->disp = modrm->_mod % 3;

    bool mod3 = false;
    if (modrm->_mod == 3) {
        mod3 = true;
        modrm->rm = &this->regs[modrm->_rm];

        goto detType;
    }

    switch (modrm->_rm) {
        case 0: modrm->sib.idx = BX; modrm->sib.base = SI; break;
        case 1: modrm->sib.idx = BX; modrm->sib.base = DI; break;
        case 2: modrm->sib.idx = BP; modrm->sib.base = SI; break;
        case 3: modrm->sib.idx = BP; modrm->sib.base = DI; break;
        case 4: modrm->rm = SI; break;
        case 5: modrm->rm = DI; break;
        case 6: modrm->rm = BP; break;
        case 7: modrm->rm = BX; break;
    }
    if (modrm->_mod == 0 && modrm->_rm == 6) {
        modrm->rm = nullptr;
        modrm->disp = 2;
    }
    
    modrm->rm_type  = RegType::R16;
    modrm->sib.idx_type  = RegType::R16;
    modrm->sib.base_type = RegType::R16;
    if (this->extra_info.contains("ad")) {
        modrm->rm_type  = RegType::R32;
        modrm->sib.idx_type  = RegType::R32;
        modrm->sib.base_type = RegType::R32;
    }

detType:
    switch (type) {
        default: break;

        case RegType::R8: case RegType::R8H:
            modrm->reg_type = RegType::R8;

            if (modrm->_reg >= 4 && modrm->_reg < 8) {
                modrm->_reg -= 4; modrm->reg_type = RegType::R8H;
            }
            modrm->reg = &this->regs[modrm->_reg];
            break;

        case RegType::R16: case RegType::R32:
            modrm->reg_type = RegType::R16;

            if (this->extra_info.contains("op")) {
                modrm->reg_type = RegType::R32;
            }
            modrm->reg = &this->regs[modrm->_reg];
    }

    if (mod3) {
        modrm->rm_type = modrm->reg_type;
    }

    return modrm;
}

ModRM *CPU::getModRM32(RegType type) {
    u8 val = this->read();
    ModRM *modrm = new ModRM(
        getMask(val, 7, 6),
        getMask(val, 5, 3),
        getMask(val, 2, 0)
    );

    switch (modrm->_mod) {
        default: modrm->disp = 0; break;
        case 1:  modrm->disp = 1; break;
        case 2:  modrm->disp = 4; break;
    }

    if (modrm->_mod == 3) {
        this->determineModRMMod3(modrm, type);
        return modrm;
    }

    if (modrm->_rm == 4) {
        this->determineModRMSib(modrm, type, this->read());
        return modrm;
    }

    if (modrm->_rm == 5 && modrm->_mod == 0) {
        modrm->rm = IP;
        modrm->disp = 4;
    }

    this->determineModRMMod0to2(modrm, type);
    return modrm;
}

ModRM *CPU::getModRM(RegType type) {
    if (!CR0->pe) {
        return this->getModRM16(type);
    } else {
        return this->getModRM32(type);
    }
}

u64 CPU::getModRMPtr(ModRM *modrm, u32 &disp) {
    u64 val = 0;

    if (!modrm->shouldUseSib()) {
        val += regToMaxSize(this->toReg(modrm->rm)->get(modrm->rm_type));
    } else {
        if (modrm->sib.idx) {
            val += regToMaxSize(this->toReg(modrm->sib.idx)->get(modrm->sib.idx_type));
            val *= modrm->sib.mul;
        }
        if (modrm->sib.base) {
            val += regToMaxSize(this->toReg(modrm->sib.base)->get(modrm->sib.base_type));
        }
    }

    switch (modrm->disp) {
        case 0: break;
        case 1: val += disp = this->getVal8();  break;
        case 2: val += disp = this->getVal16(); break;
        case 4: val += disp = this->getVal32(); break;
    }

    return val;
}

void CPU::debugPrintRegs() {
    for (int i = 0; i < 0x10; i += 4) {
        std::cout << std::setw(3) << std::setfill(' ') << getRegName(i + 0, RegType::R64) << ": "
                  << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << this->regs[i + 0].r << "  "
                  << std::setw(3) << std::setfill(' ') << getRegName(i + 1, RegType::R64) << ": "
                  << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << this->regs[i + 1].r << "  "
                  << std::setw(3) << std::setfill(' ') << getRegName(i + 2, RegType::R64) << ": "
                  << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << this->regs[i + 2].r << "  "
                  << std::setw(3) << std::setfill(' ') << getRegName(i + 3, RegType::R64) << ": "
                  << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << this->regs[i + 3].r
                  << std::endl;
    }
}

#define SET(n) t[0x##n] = &CPU::OP_##n;
#define SET0F(n) t[0x##n] = &CPU::OP_0F_##n;

static constexpr std::array<bool (CPU::*)(), 0x100> make_opcode_table() {
    std::array<bool (CPU::*)(), 0x100> t{};

    SET(00)SET(01)SET(02)SET(03)SET(04)SET(05)SET(06)SET(07)SET(08)SET(09)SET(0A)SET(0B)SET(0C)SET(0D)SET(0E)SET(0F)
    SET(10)SET(11)SET(12)SET(13)SET(14)SET(15)SET(16)SET(17)SET(18)SET(19)SET(1A)SET(1B)SET(1C)SET(1D)SET(1E)SET(1F)
    SET(20)SET(21)SET(22)SET(23)SET(24)SET(25)SET(26)SET(27)SET(28)SET(29)SET(2A)SET(2B)SET(2C)SET(2D)SET(2E)SET(2F)
    SET(30)SET(31)SET(32)SET(33)SET(34)SET(35)SET(36)SET(37)SET(38)SET(39)SET(3A)SET(3B)SET(3C)SET(3D)SET(3E)SET(3F)
    SET(40)SET(41)SET(42)SET(43)SET(44)SET(45)SET(46)SET(47)SET(48)SET(49)SET(4A)SET(4B)SET(4C)SET(4D)SET(4E)SET(4F)
    SET(50)SET(51)SET(52)SET(53)SET(54)SET(55)SET(56)SET(57)SET(58)SET(59)SET(5A)SET(5B)SET(5C)SET(5D)SET(5E)SET(5F)
    SET(60)SET(61)SET(62)SET(63)SET(64)SET(65)SET(66)SET(67)SET(68)SET(69)SET(6A)SET(6B)SET(6C)SET(6D)SET(6E)SET(6F)
    SET(70)SET(71)SET(72)SET(73)SET(74)SET(75)SET(76)SET(77)SET(78)SET(79)SET(7A)SET(7B)SET(7C)SET(7D)SET(7E)SET(7F)
    SET(80)SET(81)SET(82)SET(83)SET(84)SET(85)SET(86)SET(87)SET(88)SET(89)SET(8A)SET(8B)SET(8C)SET(8D)SET(8E)SET(8F)
    SET(90)SET(91)SET(92)SET(93)SET(94)SET(95)SET(96)SET(97)SET(98)SET(99)SET(9A)SET(9B)SET(9C)SET(9D)SET(9E)SET(9F)
    SET(A0)SET(A1)SET(A2)SET(A3)SET(A4)SET(A5)SET(A6)SET(A7)SET(A8)SET(A9)SET(AA)SET(AB)SET(AC)SET(AD)SET(AE)SET(AF)
    SET(B0)SET(B1)SET(B2)SET(B3)SET(B4)SET(B5)SET(B6)SET(B7)SET(B8)SET(B9)SET(BA)SET(BB)SET(BC)SET(BD)SET(BE)SET(BF)
    SET(C0)SET(C1)SET(C2)SET(C3)SET(C4)SET(C5)SET(C6)SET(C7)SET(C8)SET(C9)SET(CA)SET(CB)SET(CC)SET(CD)SET(CE)SET(CF)
    SET(D0)SET(D1)SET(D2)SET(D3)SET(D4)SET(D5)SET(D6)SET(D7)SET(D8)SET(D9)SET(DA)SET(DB)SET(DC)SET(DD)SET(DE)SET(DF)
    SET(E0)SET(E1)SET(E2)SET(E3)SET(E4)SET(E5)SET(E6)SET(E7)SET(E8)SET(E9)SET(EA)SET(EB)SET(EC)SET(ED)SET(EE)SET(EF)
    SET(F0)SET(F1)SET(F2)SET(F3)SET(F4)SET(F5)SET(F6)SET(F7)SET(F8)SET(F9)SET(FA)SET(FB)SET(FC)SET(FD)SET(FE)SET(FF)

    return t;
}
static constexpr std::array<bool (CPU::*)(), 0x100> make_opcode_table_0F() {
    std::array<bool (CPU::*)(), 0x100> t{};

    SET0F(00)SET0F(01)SET0F(02)SET0F(03)SET0F(04)SET0F(05)SET0F(06)SET0F(07)SET0F(08)SET0F(09)SET0F(0A)SET0F(0B)SET0F(0C)SET0F(0D)SET0F(0E)SET0F(0F)
    SET0F(10)SET0F(11)SET0F(12)SET0F(13)SET0F(14)SET0F(15)SET0F(16)SET0F(17)SET0F(18)SET0F(19)SET0F(1A)SET0F(1B)SET0F(1C)SET0F(1D)SET0F(1E)SET0F(1F)
    SET0F(20)SET0F(21)SET0F(22)SET0F(23)SET0F(24)SET0F(25)SET0F(26)SET0F(27)SET0F(28)SET0F(29)SET0F(2A)SET0F(2B)SET0F(2C)SET0F(2D)SET0F(2E)SET0F(2F)
    SET0F(30)SET0F(31)SET0F(32)SET0F(33)SET0F(34)SET0F(35)SET0F(36)SET0F(37)SET0F(38)SET0F(39)SET0F(3A)SET0F(3B)SET0F(3C)SET0F(3D)SET0F(3E)SET0F(3F)
    SET0F(40)SET0F(41)SET0F(42)SET0F(43)SET0F(44)SET0F(45)SET0F(46)SET0F(47)SET0F(48)SET0F(49)SET0F(4A)SET0F(4B)SET0F(4C)SET0F(4D)SET0F(4E)SET0F(4F)
    SET0F(50)SET0F(51)SET0F(52)SET0F(53)SET0F(54)SET0F(55)SET0F(56)SET0F(57)SET0F(58)SET0F(59)SET0F(5A)SET0F(5B)SET0F(5C)SET0F(5D)SET0F(5E)SET0F(5F)
    SET0F(60)SET0F(61)SET0F(62)SET0F(63)SET0F(64)SET0F(65)SET0F(66)SET0F(67)SET0F(68)SET0F(69)SET0F(6A)SET0F(6B)SET0F(6C)SET0F(6D)SET0F(6E)SET0F(6F)
    SET0F(70)SET0F(71)SET0F(72)SET0F(73)SET0F(74)SET0F(75)SET0F(76)SET0F(77)SET0F(78)SET0F(79)SET0F(7A)SET0F(7B)SET0F(7C)SET0F(7D)SET0F(7E)SET0F(7F)
    SET0F(80)SET0F(81)SET0F(82)SET0F(83)SET0F(84)SET0F(85)SET0F(86)SET0F(87)SET0F(88)SET0F(89)SET0F(8A)SET0F(8B)SET0F(8C)SET0F(8D)SET0F(8E)SET0F(8F)
    SET0F(90)SET0F(91)SET0F(92)SET0F(93)SET0F(94)SET0F(95)SET0F(96)SET0F(97)SET0F(98)SET0F(99)SET0F(9A)SET0F(9B)SET0F(9C)SET0F(9D)SET0F(9E)SET0F(9F)
    SET0F(A0)SET0F(A1)SET0F(A2)SET0F(A3)SET0F(A4)SET0F(A5)SET0F(A6)SET0F(A7)SET0F(A8)SET0F(A9)SET0F(AA)SET0F(AB)SET0F(AC)SET0F(AD)SET0F(AE)SET0F(AF)
    SET0F(B0)SET0F(B1)SET0F(B2)SET0F(B3)SET0F(B4)SET0F(B5)SET0F(B6)SET0F(B7)SET0F(B8)SET0F(B9)SET0F(BA)SET0F(BB)SET0F(BC)SET0F(BD)SET0F(BE)SET0F(BF)
    SET0F(C0)SET0F(C1)SET0F(C2)SET0F(C3)SET0F(C4)SET0F(C5)SET0F(C6)SET0F(C7)SET0F(C8)SET0F(C9)SET0F(CA)SET0F(CB)SET0F(CC)SET0F(CD)SET0F(CE)SET0F(CF)
    SET0F(D0)SET0F(D1)SET0F(D2)SET0F(D3)SET0F(D4)SET0F(D5)SET0F(D6)SET0F(D7)SET0F(D8)SET0F(D9)SET0F(DA)SET0F(DB)SET0F(DC)SET0F(DD)SET0F(DE)SET0F(DF)
    SET0F(E0)SET0F(E1)SET0F(E2)SET0F(E3)SET0F(E4)SET0F(E5)SET0F(E6)SET0F(E7)SET0F(E8)SET0F(E9)SET0F(EA)SET0F(EB)SET0F(EC)SET0F(ED)SET0F(EE)SET0F(EF)
    SET0F(F0)SET0F(F1)SET0F(F2)SET0F(F3)SET0F(F4)SET0F(F5)SET0F(F6)SET0F(F7)SET0F(F8)SET0F(F9)SET0F(FA)SET0F(FB)SET0F(FC)SET0F(FD)SET0F(FE)SET0F(FF)

    return t;
}

const std::array<bool (CPU::*)(), 0x100> CPU::opcode_table = make_opcode_table();
const std::array<bool (CPU::*)(), 0x100> CPU::opcode_table_0F = make_opcode_table_0F();