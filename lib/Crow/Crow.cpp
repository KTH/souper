#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/KnownBits.h"
#include "souper/Crow/Crow.h"

using namespace llvm;
extern unsigned DebugLevel;

namespace souper {


    static cl::opt<std::string> CROWSocketHost(
        "souper-crow-host", cl::Hidden,
        llvm::cl::init("127.0.0.1"), llvm::cl::value_desc("CROW bridge host ip"));


    static cl::opt<unsigned> CROWPort("souper-crow-port", cl::Hidden,
        cl::init(56789),
        cl::desc("CROW socket port"));


    static cl::opt<bool> CROW("souper-crow", cl::init(false),
        cl::desc("Get all possible replacements for randomization with CROW"));

    static llvm::cl::opt<std::string> SouperSubset(
        "souper-subset", cl::Hidden,
        llvm::cl::init(""), llvm::cl::value_desc("Subset to be applied as candidates: 1,3,6"));


    static cl::opt<unsigned> CROWWorkers("souper-crow-workers", cl::Hidden,
        cl::init(1),
        cl::desc("Number of paralleling inferring to get valid replacements"));


    void rk_sema_destroy(struct rk_sema *s)
    {
    #ifdef __APPLE__
        dispatch_semaphore_t *sem = &s->sem;

        dispatch_release(*sem);
    #else
        sem_destroy(&s->sem);
    #endif
    }

    void rk_sema_wait(struct rk_sema *s)
    {

    #ifdef __APPLE__
        dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
    #else
        int r;

        do {
                r = sem_wait(&s->sem);
        } while (r == -1 && errno == EINTR);
    #endif
    }

    void rk_sema_post(struct rk_sema *s)
    {

    #ifdef __APPLE__
        dispatch_semaphore_signal(s->sem);
    #else
        sem_post(&s->sem);
    #endif
    }

    void rk_sema_init(struct rk_sema *s, uint32_t value)
    {
    #ifdef __APPLE__
        dispatch_semaphore_t *sem = &s->sem;

        *sem = dispatch_semaphore_create(value);
    #else
        sem_init(&s->sem, 1, value);
    #endif
    }

    // Define the static Singleton pointer
    CROWSocketBridge* CROWSocketBridge::inst_ = NULL;

    CROWSocketBridge* CROWSocketBridge::getInstance() {
        if (inst_ == NULL) {
            inst_ = new CROWSocketBridge();
        }
        return(inst_);
    }

    void CROWSocketBridge::init(){

        if(DebugLevel > 2)
            errs() << "Opening socket server\n";
      
        int opt = 1;

        address.sin_family = AF_INET; 
        address.sin_port = htons(CROWPort);

        int validIp = inet_pton(AF_INET, CROWSocketHost.c_str(), &address.sin_addr);
      
        if(validIp){
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            opened = true;
            connect(sockfd, (struct sockaddr *)&address, sizeof(address));
        }
        
    }

    void CROWSocketBridge::sendKVPair(std::string key, std::string replacement){

        std::string keyValuePair = "{\"key\": \"" +  key;
        keyValuePair = keyValuePair + "\", \"value\": \"" + replacement;
        keyValuePair = keyValuePair +  + "\"},";

        send(sockfd , keyValuePair.data(), keyValuePair.size() , 0 );
    }
}

