#include <AudioGeneratorMilena.h>
#include <AudioOutputI2S.h>

/*
 * Zakomentuj linię poniżej aby użyć wewnętrznego przetwornika
 * i podłączyć wzmacniacz do pinu 25 lub 26
 */
 
#define EXTERNAL_DAC

/*
 *  numery pinów dla zewnętrznego I2S DAC
 */
#define PIN_BCLK 12
#define PIN_WCLK 14
#define PIN_DOUT 13


float myPitch=0.8, mySpeed=1.2;
int myContrast=0;
#ifdef EXTERNAL_DAC
float myGain=0.5;
#else
float myGain=1.0;
#endif

AudioGeneratorMilena *ami;
AudioOutputI2S *out;

void speakloop(void)
{
    if (ami && ami->isRunning()) {
        if (!ami->loop()) {
            ami->stop();
            delete ami;
            ami = NULL;
        }
    }
}

bool say(const char *txt)
{
    if (ami) {
        ami->stop();
        delete ami;
        delete out;
        ami = NULL;
    }
    if (!txt) return 1;
    ami = new AudioGeneratorMilena();
    ami->setPitch(myPitch);
    ami->setSpeed(mySpeed);
#ifdef EXTERNAL_DAC
    ami->setVolume(1.0);
    out = new AudioOutputI2S(0,0);
    out->SetPinout(PIN_BCLK, PIN_WCLK, PIN_DOUT);
#else
    out = new AudioOutputI2S(0,1);
    mil->setVolume(1.4);
    mil->setInternalDAC(1);
#endif
    out->SetGain(myGain);
    ami->setContrast(myContrast);
    ami->setShowPhonemes(0);
    if (!ami->begin(txt, out)) {
        delete ami;
        delete out;
        ami = NULL;
        return false;
    }
    
    return true;
}

char buf[1024];
int bufpos;

int sercmd(const char *buf)
{
    while (*buf && isspace(*buf)) buf++;
    if (*buf++ != '\\') return 0;
    while (*buf && isspace(*buf)) buf++;
    char cmd = *buf++;
    if (!cmd) return 0;
    if (cmd == '?') {
        printf("Wysokość: %.2f\n", myPitch);
        printf("Szybkość: %.2f\n", mySpeed);
        printf("Kontrast: %d\n", myContrast);
        return 1;
    }
    cmd = tolower(cmd);
    if (strchr("spc", cmd))  {
        float f=strtod(buf, NULL);
        if (cmd == 's') {
            mySpeed=f;
            if (ami) ami->setSpeed(mySpeed = f);
        }
        else if (cmd == 'c') {
            myContrast = (int) ((f < 1.0) ? (100 * f) : f);
            if (ami) ami->setContrast(myContrast);
        }
        else {
            myPitch = f;
            if (ami) ami->setPitch(myPitch = f);
        }
        return 1;
    }
    return 0;
}

int getser(void)
{

    while (Serial.available()) {
        int z=Serial.read();
        if (z == 13 || z == 10) {
            if (!bufpos) continue;
            buf[bufpos]= 0;
            bufpos=0;
            return 1;
        }
        if (bufpos < 1023) buf[bufpos++]=z;
    }
    return 0;
}


void setup(void)
{
    Serial.begin(115200);
    bufpos=0;
    say("Dzień dobry");
}


void loop()
{
    speakloop();
    if (getser()) {
        if (!sercmd(buf)) say(buf);
    }
    
}
