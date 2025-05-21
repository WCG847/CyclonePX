#pragma once

#include <cstdint>

volatile int32_t REGISTERS[32] = { 0 };
volatile int32_t pc = 0;
volatile int32_t hi = 0;
volatile int32_t lo = 0;
volatile uint32_t COP0[32] = { 0 };

// Aliases for key COP0 registers
#define COP0_Index   COP0[0]
#define COP0_Random  COP0[1]
#define COP0_Status  COP0[12]
#define COP0_Cause   COP0[13]
#define COP0_EPC     COP0[14]

// COP0 Status Register Bit Definitions
#define STATUS_IE        (1 << 0)  // Interrupt Enable
#define STATUS_KU        (1 << 1)  // Kernel/User Mode
#define STATUS_IEc       (1 << 2)  // Current Interrupt Enable
#define STATUS_KUc       (1 << 3)  // Current Mode
#define STATUS_IM_MASK   0x0000FF00 // Interrupt mask bits

uint32_t INSTRUCTION_MEMORY[1024] = { 0 };

void raiseException(uint32_t code, uint32_t badInstrAddr) {
	COP0_Cause = code << 2;
	COP0_EPC = badInstrAddr;
	COP0_Status |= (1 << 1); // Set EXL bit (Status[1])
	pc = 0x80000080; // Standard exception vector for PSX
}


enum MIPS {
	ZERO, AT, V0, V1, A0, A1, A2, A3,
	T0, T1, T2, T3, T4, T5, T6, T7,
	S0, S1, S2, S3, S4, S5, S6, S7,
	T8, T9, K0, K1, GP, SP, FP, RA
};

#define REG(n) REGISTERS[n]

// Core Execution Function
inline void step() {
	uint32_t instr = INSTRUCTION_MEMORY[pc++];
	uint8_t opcode = (instr >> 26) & 0x3F;

	if (opcode == 0x00) { // R-type
		uint8_t rs = (instr >> 21) & 0x1F;
		uint8_t rt = (instr >> 16) & 0x1F;
		uint8_t rd = (instr >> 11) & 0x1F;
		uint8_t shamt = (instr >> 6) & 0x1F;
		uint8_t funct = instr & 0x3F;

		switch (funct) {
		case 0x20: REG(rd) = REG(rs) + REG(rt); break; // ADD
		case 0x22: REG(rd) = REG(rs) - REG(rt); break; // SUB
		case 0x24: REG(rd) = REG(rs) & REG(rt); break; // AND
		case 0x25: REG(rd) = REG(rs) | REG(rt); break; // OR
		case 0x2A: REG(rd) = (REG(rs) < REG(rt)) ? 1 : 0; break; // SLT
		case 0x18: lo = REG(rs) * REG(rt); hi = 0; break; // MULT (lo-only sim)
		case 0x1A: REG(rd) = lo; break; // MFLO
		case 0x00: REG(rd) = REG(rt) << shamt; break; // SLL
		case 0x02: REG(rd) = REG(rt) >> shamt; break; // SRL
		}
	}
	else if (opcode == 0x08) { // ADDI
		uint8_t rt = (instr >> 16) & 0x1F;
		uint8_t rs = (instr >> 21) & 0x1F;
		int16_t imm = instr & 0xFFFF;
		REG(rt) = REG(rs) + imm;
	}
	else if (opcode == 0x0C) { // ANDI
		uint8_t rt = (instr >> 16) & 0x1F;
		uint8_t rs = (instr >> 21) & 0x1F;
		uint16_t imm = instr & 0xFFFF;
		REG(rt) = REG(rs) & imm;
	}
	else if (opcode == 0x04) { // BEQ
		uint8_t rs = (instr >> 21) & 0x1F;
		uint8_t rt = (instr >> 16) & 0x1F;
		int16_t offset = instr & 0xFFFF;
		if (REG(rs) == REG(rt))
			pc += ((int16_t)(instr & 0xFFFF)) << 2;
	}
	else if (opcode == 0x02) { // J
		uint32_t target = (instr & 0x03FFFFFF) << 2;
		pc = (pc & 0xF0000000) | target;
	}
	else if (opcode == 0x10) { // COP0
		uint8_t rs = (instr >> 21) & 0x1F;
		uint8_t rt = (instr >> 16) & 0x1F;
		uint8_t rd = (instr >> 11) & 0x1F;

		switch (rs) {
		case 0x00: // MFC0
			REG(rt) = COP0[rd];
			break;

		case 0x04: // MTC0
			COP0[rd] = REG(rt);
			break;

		case 0x0C: // SYSCALL
			raiseException(0x8, pc); // Syscall exception code
			break;

		}
	}
	// COP0 ERET
	if (opcode == 0x10 && (instr & 0x3F) == 0x18) { // ERET (special case)
		pc = COP0_EPC;
		COP0_Status &= ~(1 << 1); // Clear EXL
		return;
	}


	REGISTERS[0] = 0; // $zero is hardwired to 0
}

void checkInterrupts() {
	if ((COP0_Status & STATUS_IE) && !(COP0_Status & (1 << 1))) {
		// Pretend IRQ fired
		raiseException(0x0, pc);
	}
}
