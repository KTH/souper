#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/KnownBits.h"
#include "souper/Crow/Crow.h"

using namespace llvm;
extern unsigned DebugLevel;

bool CROW;
unsigned CROWWorkers;
unsigned CROWMaxReplacementSize;

bool CROWPruneSelect;
bool CROWPruneUnaryOperatorOnConstant;
bool CROWPruneBinaryCommutative;
bool CROWOperateOnTwoConstants;
bool CROWPruneSub;
bool CROWPruneConstantSelect;
bool CROWCheckMetadata;

std::string SouperSubset;


namespace souper {



    static cl::opt<bool, /*ExternalStorage=*/true> CROWFlag("souper-crow",
        cl::desc("Get all possible replacements for randomization with CROW"),
        cl::location(CROW),  cl::init(false));

    static llvm::cl::opt<std::string, /*ExternalStorage=*/true> SouperSubsetFlag(
        "souper-subset",
        cl::desc("Subset to be applied as candidates: 1,3,6"),
        cl::location(SouperSubset),
        cl::init(""));

    static cl::opt<unsigned, /*ExternalStorage=*/true> CROWWorkersFlags(
        "souper-crow-workers",
        cl::desc("Number of paralleling inferring to get valid replacements"),
        cl::location(CROWWorkers),
        cl::init(1));
        
    static cl::opt<bool, /*ExternalStorage=*/true> CROWCheckMetadataFlag(
        "souper-crow-check",
        cl::desc("Check only for metadata, not inferring"),
        cl::location(CROWCheckMetadata),
        cl::init(false));
        

    static cl::opt<unsigned, /*ExternalStorage=*/true> CROWMaxReplacementSizeFlags(
        "souper-crow-max-replacement-size",
        cl::desc("Maximum number of instructions in the replacement"),
        cl::location(CROWMaxReplacementSize),
        cl::init(1000));

    static cl::opt<bool, /*ExternalStorage=*/true> CROWPruneSelectFlag(
        "souper-crow-prune-select",
        cl::desc("Prune select operators in enumerative synthesis"),
        cl::location(CROWPruneSelect),
        cl::init(false));

    static cl::opt<bool, /*ExternalStorage=*/true> CROWPruneUnaryOperatorOnConstantFlag(
        "souper-crow-prune-unary-operator-on-constant",
        cl::desc("Prune unary operation on constant in enumerative synthesis"),
        cl::location(CROWPruneUnaryOperatorOnConstant),
        cl::init(false));

    static cl::opt<bool, /*ExternalStorage=*/true> CROWPruneBinaryCommutativeFlag(
        "souper-crow-prune-binary-commutative",
        cl::desc("Prune binary commutative instructions in enumerative synthesis"),
        cl::location(CROWPruneBinaryCommutative),
        cl::init(false));

    static cl::opt<bool, /*ExternalStorage=*/true> CROWOperateOnTwoConstantsFlag(
        "souper-crow-prune-2const-operation",
        cl::desc("Prune operate on two constants in enumerative synthesis"),
        cl::location(CROWOperateOnTwoConstants),
        cl::init(false));

    static cl::opt<bool, /*ExternalStorage=*/true> CROWPruneSubFlag(
        "souper-crow-prune-sub",
        cl::desc("Prune to synthesize sub x, C since this is covered by add x, -C in enumerative synthesis"),
        cl::location(CROWPruneSub),
        cl::init(false));

    static cl::opt<bool, /*ExternalStorage=*/true> CROWPruneConstantSelectFlag(
        "souper-crow-prune-constant-select",
        cl::desc("Prune select's control input should never be constant in enumerative synthesis"),
        cl::location(CROWPruneConstantSelect),
        cl::init(false));

    static cl::opt<std::string> CROWSocketHost(
        "souper-crow-host", cl::Hidden,
        llvm::cl::init("127.0.0.1"), llvm::cl::value_desc("CROW bridge host ip"));


    static cl::opt<unsigned> CROWPort("souper-crow-port", cl::Hidden,
        cl::init(56789),
        cl::desc("CROW socket port"));

    static cl::opt<unsigned> CROWSocketMaxRetry("souper-crow-socket-retries", cl::Hidden,
        cl::init(3),
        cl::desc("CROW socket send retries"));

    static cl::opt<unsigned> CROWSocketTimeout("souper-crow-socket-timeout", cl::Hidden,
        cl::init(1),
        cl::desc("CROW socket wait timeout"));

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

    int CROWSocketBridge::init(){

        // create an instance of your own connection handler
       

        if(DebugLevel > 1)
            errs() << "Opening socket server on " << CROWPort << "\n";
      
        address.sin_family = AF_INET; 
        address.sin_port = htons(CROWPort);

        int validIp = inet_pton(AF_INET, CROWSocketHost.c_str(), &address.sin_addr);
      
        if(validIp){
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if(sockfd > -1){
                opened = connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == 0;
                errs() << "Connection up and running\n";
                return 1;
            }
        }else{
            errs() << "Invaild IP " << CROWSocketHost << "\n";
        }
        
        return 0;
    }

    bool CROWSocketBridge::isOpen(){
        return opened;
    }

    int CROWSocketBridge::reconnect(){
        int error_code;
        int tries = CROWSocketMaxRetry;
        while(tries){
            error_code = init();

            if(!error_code) break;

            if(DebugLevel > 1)
                errs() << "Trying socket reconnection...\n";
            //sleep(CROWSocketTimeout);

            tries--;      
        }

        if(error_code){

            if(DebugLevel > 1)
                errs() << "Socket broken!\n";
            opened = false;
            return 0;
        }

        return 1;
    }

    void CROWSocketBridge::sendKVPair(std::string key, std::string replacement){

        if(DebugLevel > 1)
            errs() << "Sending KV pair to CROW\n";
      

        if(sockfd > -1){

            int error_code;
            socklen_t error_code_size = sizeof(error_code);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
            

            if(error_code){
                if(DebugLevel > 1)
                    errs() << "Socket broken errno:" << error_code << "\n";

                if(!reconnect()){
                    return;
                }
                
            }

            std::string keyValuePair = "{\"key\": \"" +  key;
            keyValuePair = keyValuePair + "\", \"value\": \"" + replacement;
            keyValuePair = keyValuePair +  + "\"},";

            int sent = send(sockfd , keyValuePair.data(), keyValuePair.size() , 0 );
            

        }
        else{

            if(DebugLevel > 1)
                errs() << "Socket inexisten fd:" << sockfd << "\n";
        }
    }
}

