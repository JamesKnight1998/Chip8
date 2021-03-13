#include "Chip.hpp"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <random>

const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;

uint8_t fontSet[FONTSET_SIZE] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip::Chip()
  : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
  
  // Initialize RNG
	randByte = std::uniform_int_distribution<uint8_t>(0, 255U);

  // Initialize PC
  pc = START_ADDRESS;

  for (unsigned int i = 0; i < FONTSET_SIZE; i++) {
    memory[FONTSET_START_ADDRESS + i] = fontSet[i];
  }

  // Set up function pointer table
	table[0x0] = &Chip::Table0;
	table[0x1] = &Chip::OP_1nnn;
	table[0x2] = &Chip::OP_2nnn;
	table[0x3] = &Chip::OP_3xkk;
	table[0x4] = &Chip::OP_4xkk;
	table[0x5] = &Chip::OP_5xy0;
	table[0x6] = &Chip::OP_6xkk;
	table[0x7] = &Chip::OP_7xkk;
	table[0x8] = &Chip::Table8;
	table[0x9] = &Chip::OP_9xy0;
	table[0xA] = &Chip::OP_Annn;
	table[0xB] = &Chip::OP_Bnnn;
	table[0xC] = &Chip::OP_Cxkk;
	table[0xD] = &Chip::OP_Dxyn;
	table[0xE] = &Chip::TableE;
	table[0xF] = &Chip::TableF;

	table0[0x0] = &Chip::OP_00E0;
	table0[0xE] = &Chip::OP_00EE;

	table8[0x0] = &Chip::OP_8xy0;
	table8[0x1] = &Chip::OP_8xy1;
	table8[0x2] = &Chip::OP_8xy2;
	table8[0x3] = &Chip::OP_8xy3;
	table8[0x4] = &Chip::OP_8xy4;
	table8[0x5] = &Chip::OP_8xy5;
	table8[0x6] = &Chip::OP_8xy6;
	table8[0x7] = &Chip::OP_8xy7;
	table8[0xE] = &Chip::OP_8xyE;

	tableE[0x1] = &Chip::OP_ExA1;
	tableE[0xE] = &Chip::OP_Ex9E;

	tableF[0x07] = &Chip::OP_Fx07;
	tableF[0x0A] = &Chip::OP_Fx0A;
	tableF[0x15] = &Chip::OP_Fx15;
	tableF[0x18] = &Chip::OP_Fx18;
	tableF[0x1E] = &Chip::OP_Fx1E;
	tableF[0x29] = &Chip::OP_Fx29;
	tableF[0x33] = &Chip::OP_Fx33;
	tableF[0x55] = &Chip::OP_Fx55;
	tableF[0x65] = &Chip::OP_Fx65;
}

void Chip::LoadROM(char const* fileName) {
  // Chip::Open the file as a stream of binary and move the file pointer to the end
	std::ifstream file(fileName, std::ios::binary | std::ios::ate);

	if (file.is_open()) {
		// Get size of file and allocate a buffer to hold the contents
    std::streampos size = file.tellg();
    char *buffer = new char[size];

    // Go back to the beginning of the file and fill the buffer
		file.seekg(0, std::ios::beg);
		file.read(buffer, size);
    file.close();

    // Load the ROM contents into the Chip's memory, starting at 0x200
		for (long i = 0; i < size; ++i) {
			memory[START_ADDRESS + i] = buffer[i];
		}

		// Free the buffer
		delete[] buffer;
	}
}

void Chip::Cycle()
{
	// Fetch
	opcode = (memory[pc] << 8u) | memory[pc + 1];

	// Increment the PC before we execute anything
	pc += 2;

	// Decode and Execute
	((*this).*(table[(opcode & 0xF000u) >> 12u]))();

	// Decrement the delay timer if it's been set
	if (delayTimer > 0) {
		--delayTimer;
	}

	// Decrement the sound timer if it's been set
	if (soundTimer > 0) {
		--soundTimer;
	}
}


// Do nothing
void Chip::OP_NULL() {
  
}

// CLS
void Chip::OP_00E0() {
  memset(video, 0, sizeof(video));
}

// RET
void Chip::OP_00EE() {
  pc = stack[sp--];
}

// JP address
void Chip::OP_1nnn() {
  uint16_t nnn = opcode & 0x0FFFu;
  pc = nnn;
}

// CALL address
void Chip::OP_2nnn() {
  uint16_t nnn = opcode & 0x0FFFu;

  stack[sp++] = pc;
  pc = nnn;
}

// SE Vx, byte
void Chip::OP_3xkk() {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t kk = opcode & 0x00FFu;

	if (registers[Vx] == kk) {
		pc += 2;
	}
}

// SNE Vx, byte
void Chip::OP_4xkk() {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t kk = opcode & 0x00FFu;

	if (registers[Vx] != kk) {
		pc += 2;
	}
}

// SE Vx, Vy
void Chip::OP_5xy0() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] == registers[Vy]) {
		pc += 2;
	}
}

// LD Vx, byte
void Chip::OP_6xkk() {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t kk = opcode & 0x00FFu;

  registers[Vx] = kk;
}

// ADD Vx, byte
void Chip::OP_7xkk() {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t kk = opcode & 0x00FFu;

  registers[Vx] += kk;
}

// LD Vx, Vy
void Chip::OP_8xy0() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  registers[Vx] = registers[Vy];
}

// OR Vx, Vy
void Chip::OP_8xy1() {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  registers[Vx] |= registers[Vy];
}

// AND Vx, Vy
void Chip::OP_8xy2() {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  registers[Vx] &= registers[Vy];
}

// XOR Vx, Vy
void Chip::OP_8xy3() {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  registers[Vx] ^= registers[Vy];
}

// ADD Vx, Vy
void Chip::OP_8xy4() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	uint16_t sum = registers[Vx] + registers[Vy];

	if (sum > 255U) {
		registers[0xF] = 1;
	} else {
		registers[0xF] = 0;
	}

	registers[Vx] = sum & 0xFFu;
}

// SUB Vx, Vy
void Chip::OP_8xy5() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] > registers[Vy]) {
		registers[0xF] = 1;
	} else {
		registers[0xF] = 0;
	}

	registers[Vx] -= registers[Vy];
}

// SHR Vx
void Chip::OP_8xy6() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	// Save LSB in VF
	registers[0xF] = (registers[Vx] & 0x1u);

	registers[Vx] >>= 1;
}

// SUBN Vx, Vy
void Chip::OP_8xy7() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vy] > registers[Vx]) {
		registers[0xF] = 1;
	} else {
		registers[0xF] = 0;
	}

	registers[Vx] = registers[Vy] - registers[Vx];
}

// SHL Vx
void Chip::OP_8xyE() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	// Save MSB in VF
	registers[0xF] = (registers[Vx] & 0x80u) >> 7u;

	registers[Vx] <<= 1;
}

// SNE Vx, Vy
void Chip::OP_9xy0() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] != registers[Vy]) {
		pc += 2;
	}
}

// LD I, address
void Chip::OP_Annn() {
	uint16_t nnn = opcode & 0x0FFFu;

	index = nnn;
}

// JP V0, address
void Chip::OP_Bnnn() {
	uint16_t nnn = opcode & 0x0FFFu;

  pc = registers[0] + nnn;
}

// RND Vx, byte
void Chip::OP_Cxkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t kk = opcode & 0x00FFu;

	registers[Vx] = randByte(randGen) & kk;
}

// DRW Vx, Vy, height
void Chip::OP_Dxyn() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	uint8_t height = opcode & 0x000Fu;

	// Wrap if going beyond screen boundaries
	uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
	uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

	registers[0xF] = 0;

	for (unsigned int row = 0; row < height; ++row) {
		uint8_t spriteByte = memory[index + row];

		for (unsigned int col = 0; col < 8; ++col) {
			uint8_t spritePixel = spriteByte & (0x80u >> col);
			uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

			// Sprite pixel is on
			if (spritePixel) {
				// Screen pixel also on - collision
				if (*screenPixel == 0xFFFFFFFF) {
					registers[0xF] = 1;
				}

				// Effectively XOR with the sprite pixel
				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}
}

// SKP Vx
void Chip::OP_Ex9E() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t key = registers[Vx];

	if (keypad[key]) {
		pc += 2;
	}
}

// SKNP Vx
void Chip::OP_ExA1() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t key = registers[Vx];

	if (!keypad[key]) {
		pc += 2;
	}
}

// LD Vx, DT
void Chip::OP_Fx07() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	registers[Vx] = delayTimer;
}

// LD Vx, K
void Chip::OP_Fx0A() {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  bool pressed = false;

  for (unsigned int i = 0; i < KEY_COUNT; i++) {
    if (keypad[i]) {
      registers[Vx] = i;
      pressed = true;

      break;
    }
  }

  if (!pressed) {
    pc -= 2;
  }
}

// LD DT, Vx
void Chip::OP_Fx15() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	delayTimer = registers[Vx];
}

// LD ST, Vx
void Chip::OP_Fx18() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	soundTimer = registers[Vx];
}

// ADD I, Vx
void Chip::OP_Fx1E() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	index += registers[Vx];
}

// LD F, Vx
void Chip::OP_Fx29() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t digit = registers[Vx];

	index = FONTSET_START_ADDRESS + (5 * digit);
}

// LD B, Vx
void Chip::OP_Fx33() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t value = registers[Vx];

	// Ones-place
	memory[index + 2] = value % 10;
	value /= 10;

	// Tens-place
	memory[index + 1] = value % 10;
	value /= 10;

	// Hundreds-place
	memory[index] = value % 10;
}

// LD [I], Vx
void Chip::OP_Fx55() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i) {
		memory[index + i] = registers[i];
	}
}

// LD Vx, [I]
void Chip::OP_Fx65() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i) {
		registers[i] = memory[index + i];
	}
}

void Chip::Table0() {
	((*this).*(table0[opcode & 0x000Fu]))();
}

void Chip::Table8() {
	((*this).*(table8[opcode & 0x000Fu]))();
}

void Chip::TableE() {
	((*this).*(tableE[opcode & 0x000Fu]))();
}

void Chip::TableF() {
	((*this).*(tableF[opcode & 0x00FFu]))();
}