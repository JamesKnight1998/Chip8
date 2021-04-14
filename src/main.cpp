#include "Chip.hpp"
#include "View.hpp"
#include <chrono>
#include <iostream>

int main(int argc, char** argv) {
	//Check command-line arguments
	if (argc != 4) {
		std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
		std::exit(EXIT_FAILURE);
	}

	//Initialise argument variables
	int videoScale = std::stoi(argv[1]);
	int cycleDelay = std::stoi(argv[2]);
	char const* romFilename = argv[3];

	//Create SDL2 screen handler
	View view("CHIP-8 Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);

	//Create Chip8 and load rm
	Chip chip;
	chip.LoadROM(romFilename);

	int videoPitch = sizeof(chip.video[0]) * VIDEO_WIDTH;

	//Set the previous cycle time
	auto lastCycleTime = std::chrono::high_resolution_clock::now();
	bool quit = false;

	while (!quit) {
		quit = view.ProcessInput(chip.keypad);

		//Set current cycle time
		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();

		//Step to next CPU cycle if enough time has passed and update the screen
		if (dt > cycleDelay) {
			lastCycleTime = currentTime;

			chip.Cycle();

			view.Update(chip.video, videoPitch);
		}
	}

	return 0;
}