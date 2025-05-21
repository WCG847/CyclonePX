#pragma once
#pragma message("✔ CORRECT IOPORT.H LOADED")

namespace Emu {
	extern volatile int VOLATILE_BUS_BLOCK[1024];
	constexpr int COM_DELAY = (0x20 / 4);
}
