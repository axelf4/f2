#include <net.h>
#include <thread>

namespace game {

	class Server {
		struct peer *server;
		std::thread inputThread;

		void loopRead();
	public:
		Server();
		~Server();

		bool continuing;

		void readPeer();

		void startServer();
	};
}