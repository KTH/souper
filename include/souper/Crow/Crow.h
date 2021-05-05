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
#include "souper/Inst/Inst.h"

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
			void sendKVPair(std::string key, std::string replacement, unsigned blockId);
			void replace_check(std::function<bool(CROWSocketBridge*, Inst *)> new_func)
			{
				check_function = new_func;
			}

			bool check(Inst * instr){
				return check_function(this, instr);
			}


			static std::map<std::string, std::string> inlineCache;
			
		protected:
			int sockfd = -1;
			bool opened = false;
			struct sockaddr_in address; 
			CROWSocketBridge() : sockfd(-1),opened(false) {} // private constructor

		private:
			static CROWSocketBridge* inst_;   // The one, single instance
			CROWSocketBridge(const CROWSocketBridge&);
			CROWSocketBridge& operator=(const CROWSocketBridge&);
			bool check_default(Inst *I){
				return true;
			}
			std::function<bool(CROWSocketBridge*, Inst *)> check_function = &CROWSocketBridge::check_default;


	};

}

#endif
