#include <AudioGeneratorMilena.h>
#include <AudioOutputI2S.h>
#include <WiFi.h>
#include <Bounce2.h>
#include <time.h>

/*
 * Based on code from:
 * https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/
 */

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

/*
 * Pin wyzwalania odczytu - 0 dla wbudowanego pinu BOOT
 * o ile taki istnieje na płytce
 */

#define PIN_KEY 0

/*
 * SSID i hasło WiFi
 */

const char *MySSID = "*****";
const char *MyPASS = "*****";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;


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

Bounce bounce;

void sayLocalTime(bool full=false)
{
    struct tm tm;
    static const char *dt[]={"niedziela","poniedziałek","wtorek","środa",
            "czwartek","piątek","sobota"};
    char buf[128];
    if (!getLocalTime(&tm)) {
        say("Nie mogę ustalić godziny");
        return;
    }
    if (full)
        sprintf(buf,"Jest %s, %02d.%02d.%04d, %02d:%02d", dt[tm.tm_wday],tm.tm_mday, tm.tm_mon+1, tm.tm_year+1900,tm.tm_hour, tm.tm_min);
    else
        sprintf(buf,"%02d:%02d", tm.tm_hour, tm.tm_min);
    say(buf);    
}

void setup(void)
{
    Serial.begin(115200);
    say("Łączę z serwerem");
    while (ami && ami->isRunning()) speakloop();
    WiFi.mode(WIFI_STA);
    WiFi.begin(MySSID, MyPASS);
    while(WiFi.status() != WL_CONNECTED) {
        printf(".");
        delay(500);
    }
    printf("połączone\n");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    sayLocalTime(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    bounce.attach(PIN_KEY, INPUT_PULLUP);
    bounce.interval(100);
}



void loop()
{
    speakloop();
    bounce.update();
    if (bounce.rose()) {
        sayLocalTime(bounce.previousDuration() > 500);
    }
}
