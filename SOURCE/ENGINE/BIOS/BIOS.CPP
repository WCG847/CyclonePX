#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "BIOS.H"
#include "SCRATCHPAD.H"
#include "MIPS.h"
#include "Region.h"
#include "IOPORT.h"
#include "CPU/CPU.H"



constexpr size_t BIOS_SIZE = 512 * 1024;     
constexpr size_t BIOS_MAX_SIZE = 4096 * 1024;

uint8_t* biosMem = nullptr;

uint8_t* WriteInfo(uint8_t*biosMem) {
	BIOS* biosHeader = reinterpret_cast<BIOS*>(biosMem);
	biosHeader->day = 0x4;
	biosHeader->month = 0x12;
	biosHeader->year = 0x1995;
	biosHeader->consoleType = 0x00000003;

	strcpy(biosHeader->CompanyName, "Sony Computer Entertainment Inc.");
	strcpy(biosHeader->makerID, "CEX-3000/1001/1002 by K.S.");
	return biosMem;
}

void WriteIOPorts(uint8_t* biosMem, volatile int* IOPorts, volatile int* ExpansionRegion1, volatile int* ExpansionRegion2) {
	IOPorts[Emu::COM_DELAY] = 0x31125;
	IOPorts[0] = (uintptr_t)ExpansionRegion1;
	IOPorts[1] = (uintptr_t)ExpansionRegion2;
	IOPorts[2] = 0x0013243;
	IOPorts[5] = 0x200931E1;
	IOPorts[6] = 0x00020843;
	IOPorts[3] = 0x00003022;
	IOPorts[7] = 0x00070777;

}

void WriteToCPU(uint8_t* biosMem, volatile int* CACHECONTROL, int* Ring3) {
	CACHECONTROL[76] = 0x00000804;
	REG(T4) = 0x00010000;

	// Simulate: mtc0 t4, Status => COP0_Status = REG(T4)
	COP0_Status = REG(T4);

	// Clear parts of Ring 3
	memset(Ring3, 0, 524288);
}

int main() {
	biosMem = (uint8_t*)malloc(BIOS_SIZE);
	int* Ring3 = (int*)malloc(524288);
	volatile int* IOPorts = Emu::VOLATILE_BUS_BLOCK;
	volatile int* ExpansionRegion1 = EXPANSION_REGION_1;
	volatile int* ExpansionRegion2 = EXPANSION_REGION_2;
	volatile int* CACHECONTROL = CacheControl;

	WriteInfo(biosMem);
	WriteIOPorts(biosMem, IOPorts, ExpansionRegion1, ExpansionRegion2);
	WriteToCPU(biosMem, CACHECONTROL, Ring3);
}