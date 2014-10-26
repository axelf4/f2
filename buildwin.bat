mkdir build
cd build
cmake .. -G "Visual Studio 12 2013"^
	-DGLEW_ROOT_DIR="D:/Program Files/glew-1.11.0"^
	-DSOIL_ROOT_DIR="D:/Program Files/Simple OpenGL Image Library"^
	-DGLM_ROOT_DIR="D:\Program Files\glm-0.9.5.4\glm"^
	-DBULLET_ROOT="D:\Program Files\BulletBuild"^
	-DASSIMP_ROOT_DIR="D:\Program Files\assimp-3.1.1-win-binaries"

PAUSE