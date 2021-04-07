FROM ubuntu:bionic

RUN apt-get update && apt-get install -y build-essential apt-utils autoconf libtool pkg-config

#install sdl2
RUN apt install libsdl2-dev libsdl2-2.0-0 -y;

#install sdl image
RUN apt install libjpeg-dev libwebp-dev libtiff5-dev libsdl2-image-dev libsdl2-image-2.0-0 -y;

WORKDIR /src
COPY * ./

RUN g++ -o Chip8 *.cpp -lSDL2
RUN ls -a
CMD ["./Chip8", "10", "2", "./maze_alt.ch8"]