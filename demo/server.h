#include <net.h>
#include <thread>

namespace game {

	class Server {
		struct peer *server;
		std::thread inputThread;

		void readPeer();
	public:
		~Server();

		bool continuing;

		void startServer();
	};
}