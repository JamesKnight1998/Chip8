# Chip8

Simple Implementation of a Chip-8 Interpreter. Written entirely in C++ using SDL2 for video rendering.

## Compilation Instructions

1. Clone the repository

2. Open repository in a terminal

3. Run `g++ -o Chip8 src/*.cpp -lSDL2`

## Execution

Execution format is: `./Chip8 <scale> <speed> <rom>`

- e.g `./Chip8 10 5 src/maze_alt.ch8`

## Docker

This repository also contains a Dockerfile image.

To run the image use `docker run --net=host --env="DISPLAY" --volume="$HOME/.Xauthority:/root/.Xauthority:rw" chip8_quick_start`

- If you get a permissions error then run the above command in administrator mode
