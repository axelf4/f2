:: When running these arguments needs to be used for Bullet and OpenCL on the CPU:
:: --cl_platform=x --allow_opencl_cpu --use_large_batches --use_uniform_grid

mkdir build
cd build
cmake .. -G "Visual Studio 12 2013"^
	-DGLEW_ROOT_DIR="D:/Program Files/glew-1.11.0"^
	-DSOIL_ROOT_DIR="D:/Program Files/Simple OpenGL Image Library"^
	-DGLM_ROOT_DIR="D:\Program Files\glm-0.9.5.4\glm"^
	-DBULLET_ROOT="D:\Program Files\BulletBuild"^
	-DASSIMP_ROOT_DIR="D:\Program Files\assimp-3.1.1-win-binaries"^
	-DOpenCL_INCPATH="D:\Program Files (x86)\Intel\OpenCL SDK\4.6\include"^
	-DOpenCL_LIBPATH="D:\Program Files (x86)\Intel\OpenCL SDK\4.6\lib\x86"

if errorlevel 1 pause