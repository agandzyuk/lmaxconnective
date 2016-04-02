#ifndef __mqlbridge_h__
#define __mqlbridge_h__

#include <QSharedMemory>

#include <windows.h>
#include <vector>

#define MEMORY_SHARING_NAME     "fixmemfilemap"

#define MAX_PROVIDERS           2
#define MAX_PROVIDER_LENGTH     15
#define MAX_SYMBOLS             300
#define MAX_SYMBOL_LENGTH       30

struct Quote {
    double bid_;
	double ask_;
	double last_;
};

struct Provider {
	char  symbols_[MAX_SYMBOLS * MAX_SYMBOL_LENGTH];
	Quote qoutes_[MAX_SYMBOLS];
};

struct SharedData {
	char     providers_[MAX_PROVIDERS * MAX_PROVIDER_LENGTH];
	Provider data_[MAX_PROVIDERS];
};

/// 
class MQLBridge : private QSharedMemory
{
protected:
    SharedData* sh;
    int recurse_;

private:
    class Locker {
    public:
        Locker(MQLBridge* owner) 
            : owner_(*owner)
        {
            if( owner_.recurse_ == 0 ) {
                owner_.lock();
                owner_.sh = reinterpret_cast<SharedData*>(owner_.data());
            }
            owner_.recurse_++;
        }
        ~Locker() 
        {
            owner_.recurse_--;
            if( owner_.recurse_ == 0 ) {
                owner_.unlock(); 
                owner_.sh = 0;
            }
        }
    private:
        MQLBridge& owner_;
    };

public:
    MQLBridge();
    ~MQLBridge();

    bool attach();
    void detach();
    bool isAttached();
    void sendMessage(char* symbol, const short type);

    bool    getSymbols(std::vector<std::string>& symbols);
    double  getBid(const char* symbol);
    double  getAsk(const char* symbol);
    void    setBid(const char* symbol, double value);
    void    setAsk(const char* symbol, double value);

protected:
    int addProvider(const char* provider);
    int findProvider(const char* provider);
    int addSymbol(const int providerId, const char* symbol);
    int findSymbol(const int providerId, const char* symbol);

    // Accessing methods
    char*       provider(int providerNum);
    Provider&   providerData(int providerNum);
    char*       symbol(int providerNum, int symbolNum);
};

Q_GLOBAL_STATIC(MQLBridge, spMqlBridge)

#endif __mqlbridge_h__
