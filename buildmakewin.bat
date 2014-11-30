make all CFLAGS="-Wall -I./include -IC:/ProgramFiles/Libraries/include" \
	CXXFLAGS="-std=c++11 -Wall -I./include -IC:/ProgramFiles/Libraries/include" \
	LDFLAGS="-fPIC -LC:/ProgramFiles/Libraries/lib" \
	LDLIBS="./libf2.a -lSDL2 -lSDL2main -lSOIL -lassimp -lBulletDynamics -lBulletCollision -lLinearMath -lglew32 -lopengl32 \
	C:\ProgramFiles\Libraries\lib\Bullet3Dynamics_vs2010_x64_release.lib \
	C:\ProgramFiles\Libraries\lib\Bullet3Collision_vs2010_x64_release.lib \
	C:\ProgramFiles\Libraries\lib\LinearMath_vs2010_x64_release.lib"