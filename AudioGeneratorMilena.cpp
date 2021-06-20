#include "AudioGeneratorMilena.h"
#include "common.h"

extern "C" {
    static int getMilenaLine(char *buf, int size, void *userData);
};

static int getMilenaLine(char *buf, int size, void *userData)
{
    if (!userData) return 0;
    
    int n=microlena_getPhonemes((struct microlena_Buffer *)userData, buf);
    if (n > 0 && ((struct microlena_Buffer *)userData) ->show_phonemes) printf("%s\n",buf); 
    return n;
}

AudioGeneratorMilena::~AudioGeneratorMilena()
{
    if (milena) free(milena);
}
static bool isUTF8(const char *txt)
{
    uint8_t znak,n;
    bool is8bit=false;
    while((znak = (uint8_t )*txt++)) {
        if (!(znak & 0x80)) continue;
        is8bit=true;
        if ((znak & 0xe0) == 0xc0) n=1;
        else if ((znak & 0xf0) == 0xe0) n=2;
        else if ((znak & 0xf8) == 0xf0) n=3;
        else if ((znak & 0xfc) == 0xf8) n=4;
        else return false;
        while (n--) {
            if ((*txt++ & 0xc0) != 0x80) return false;
        }
    }
    return is8bit;
}

bool AudioGeneratorMilena::begin(const char *txt, AudioOutput *output)
{
    milena = microlena_InitBuffer(txt, isUTF8 (txt));
    if (!milena) return 0;
    if (flags & 1) milena->no_sub = 1;
    if (flags & 2) milena->show_phonemes = 1;
    if (flags & 4) milena->simple_decipoint = 1;
    if (decipoint[0]) strcpy(milena->decipoint, decipoint);
    if (flags & 8) milena->alt_colon = 1;
    mbrola = init_Mbrola(getMilenaLine, NULL);
    
    if (!mbrola) return 0;
    set_freq_ratio_Mbrola(mbrola, pitch);
    set_time_ratio_Mbrola(mbrola, tempo);
    set_volume_ratio_Mbrola(mbrola, vol);

    set_input_userData_Mbrola(mbrola, (void *)milena);
    bool rc= beginAudioOutput(output);
    return rc;
    
}

bool AudioGeneratorMilena::setDeciPoint(const char *txt)
{
    if (txt && strlen(txt) > 0) {
        int n = microlena_utf2iso(txt, NULL);
        if (n > 15) return false;
        microlena_utf2iso(txt, decipoint);
        return true;
    }
    return false;
}
