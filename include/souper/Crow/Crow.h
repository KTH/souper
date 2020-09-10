#ifndef CROW_H
#define CROW_H

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>


namespace souper {
	struct rk_sema {
	#ifdef __APPLE__
		dispatch_semaphore_t    sem;
	#else
		sem_t                   sem;
	#endif
	};

	void rk_sema_post(struct rk_sema *s);
	void rk_sema_destroy(struct rk_sema *s);
	void rk_sema_wait(struct rk_sema *s);
	void rk_sema_post(struct rk_sema *s);
	void rk_sema_init(struct rk_sema *s, uint32_t value);

	class CROWSocketBridge { // Singleton

		const char * addressIp = "127.0.0.1";
		
		public:
		// This is how clients can access the single instance
			static CROWSocketBridge* getInstance();
			int init();
			bool isOpen();
			int reconnect();
			void sendKVPair(std::string key, std::string replacement);

		protected:
			int sockfd = -1;
			bool opened = false;
			struct sockaddr_in address; 

		private:
			static CROWSocketBridge* inst_;   // The one, single instance
			CROWSocketBridge() : sockfd(-1),opened(false) {} // private constructor
			CROWSocketBridge(const CROWSocketBridge&);
			CROWSocketBridge& operator=(const CROWSocketBridge&);

	};

}

#endif
