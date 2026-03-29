CC = gcc
CFLAGS = -Wall -Wextra -g -I/ucrt64/include/stb -I/ucrt64/include/cjson -IE:/msys64/ucrt64/include/SDL2 -Dmain=SDL_main
LIBS = -LE:/msys64/ucrt64/lib -lmingw32 -mwindows -lSDL2main -lSDL2 -lSDL2_ttf -lglew32 -lopengl32 -lcurl -lcjson -lm

SRC = src/main.c src/sysinfo.c src/alert.c src/globe.c src/iss.c
OUT = helios.exe

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)