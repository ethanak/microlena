#include <AudioGeneratorMbrola.h>

struct microlena_Buffer;

extern "C" {
    extern void microlena_setUserDict(const char * const  *units, const char * const * dict);
};

class AudioGeneratorMilena: public AudioGeneratorMbrola
{
    public:
    AudioGeneratorMilena() : milena(NULL),udic(NULL),decipoint(""),flags(0) {};
    ~AudioGeneratorMilena();

    bool begin(const char *str, AudioOutput *outfile) override;
    void setNoSub(bool nosub) {flags = (nosub)?(flags | 1):(flags & ~1);};
    bool getNoSub(void) {return (flags & 1) != 0;};
    void setShowPhonemes(bool show) {flags = (show)?(flags | 2):(flags & ~2);};
    bool getShowPhonemes(void) {return (flags & 2) != 0;};
    void setSimpleDeciPoint(bool sd) {flags = (sd)?(flags | 4):(flags & ~4);};
    bool getSimpleDeciPoint(void) {return (flags & 4) != 0;};
    bool setDeciPoint(const char *txt);
    void setAltColon(bool alt) {flags = (alt)?(flags | 8):(flags & ~8);};
    bool getAltColon(void) {return (flags & 8) != 0;};

    enum {
        NO_SUBPHRASES = 1,
        SHOW_PHONEMES = 2,
        SIMPLE_DECIPOINT = 4,
        ALT_COLON = 8
    };
    private:
    struct microlena_Buffer *milena;
    struct microlena_userdic *udic;
    char decipoint[16];
    uint8_t flags;
    uint8_t phase;
    int16_t shocksample;
};

