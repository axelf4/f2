#include <net.h>
#include <thread>

namespace game {

	class Server {
		struct peer *server;
	public:
		Server();
		~Server();

		void update();
	};
}