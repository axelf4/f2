make -r CFLAGS="-ID:\Libraries\include" CXXFLAGS="-ID:\Libraries\include" \
	LDFLAGS="-m32 -LD:\Libraries\lib" \
	LDLIBS="-lmingw32 -lSDL2main -lSDL2 -lSOIL -lassimp -lzlibstatic -lBulletDynamics -lBulletCollision -lLinearMath ./libf2.a -lglew32 -lopengl32"
if [ $? -eq 0 ]; then
	game.exe
fi