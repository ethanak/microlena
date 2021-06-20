#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "common.h"

const char *zbeg="{}[]\\%@&$+=*&/><\260\247|#^~_`\244\367\327";

const char *znaczki[]={
"{lewa klamra",
"}prawa klamra",
"[lewy nawias kwadratowy",
"]prawy nawias kwadratowy",
"\\bekslesz",
"%procent",
"@ma\263pa",
"&end",
"$dolar",
"+plus",
"=r\363wne",
"*gwiazdka",
"&and",
"/przez",
">wi\352ksze",
"<mniejsze",
"\260stopie\361",
"\247paragraf",
"|pipa",
"#hasz",
"^daszek",
"~tylda",
"_podkre\266l",
"`gravis",
"\244waluta",
"\367dzielone",
"\327razy",NULL};

extern int microlena_integer(struct microlena_Buffer *buf, char **inps);

static int getIP4(struct microlena_Buffer *buf)
{
    char *c=buf->inptr;
    
    int i,j;
    for (i=0;i<4;i++) {
        if (!*c || !microlena_isdigit(*c)) return 0;
        for (j=0;j<3;j++) {
            if (!*c || !microlena_isdigit(c[j])) break;
        }
        if (!j) return 0;
        if (i != 3 && c[j] != '.') return 0;
        if (i == 3 && microlena_isalnum(c[j])) return 0;
        if (j > 1 && *c == '0') return 0;
        int val = strtol(c, &c, 10);
        if (val > 255) return 0;
        if (i != 3) c++;
    }
    
    for (i=0;i<4; i++) {
        if (i) {
            pushstr(buf, "kropka",1);
            buf->inptr++;
        }
        if (microlena_integer(buf, &buf->inptr)<0) return -1;
        
        //if (microlena_integer(val[i],buf) < 0) return -1;
    }
    return 1;
}

extern char *microlena_find_unit(char *str, char **ostr, uint8_t *female);

static int push_unit(struct microlena_Buffer *buf, char *unit, int typ)
{
    int j;
    for (j=0;j<typ;unit++) {
        if (*unit == '|') j++;
    }
    if (!*unit) return 1;
    if (*unit == '|') return 1;
    blank(buf);
    while (*unit && *unit != '|') pushout(buf, *unit++);
    return 1;
}

int getHour(struct microlena_Buffer *buf)
{
    int hr=0,mi=0,se=0,hs=0;
    char *c=buf->inptr, *d;
    hr=*c++-'0';
    if (microlena_isdigit(*c)) hr = 10 * hr+ *c++-'0';
    if (hr < 0 || hr > 24) return 0;
    if (*c++ != ':') return 0;
    if (!microlena_isdigit(*c)) return 0;
    mi = 10 * (*c++ - '0');
    if (!microlena_isdigit(*c)) return 0;
    mi += *c++ - '0';
    if (microlena_isalnum(*c)) return 0;
    buf->inptr=c;
    c="fm";
    if (microlena_SpeakNumberS(buf, hr, &c)<0) return -1;
    c="f0";
    if (microlena_SpeakNumberS(buf, mi, &c)<0) return -1;
    return 1;
}

static int count_digits(const char *c)
{
    int i;
    for (i=0;*c && microlena_isdigit(*c);c++, i++);
    return i;
}

int getFixed(struct microlena_Buffer *buf)
{
    char *c=buf->inptr;int vs;
    
    if (*c == '-') c++;
    if (!*c || !microlena_isdigit(*c)) return 0;
    vs=strtol(c,&c,10);
    //for (;*c && microlena_isdigit(*c);c++);
    if (*c != '.' && *c !=',') return 0;
    c++;
    if (!*c || !microlena_isdigit(*c)) return 0;
    if (microlena_integer(buf, &buf->inptr) < 0) return -1;
    if (*buf->inptr != '.' && *buf->inptr != ',') return 0;
    buf->inptr++;
    int unit_type = 3;
    if (!buf->simple_decipoint || count_digits(buf->inptr) != 1 || (*buf->inptr != '0' && *buf->inptr !='5')) {
        pushstr(buf,buf->decipoint,1);
        if (microlena_integer(buf, &buf->inptr) < 0) return -1;
    }
    else {
        if (*buf->inptr++ == '5') pushstr(buf,"i p\363\263",1);
        else {
            if (vs == 1) unit_type = 0;
            else {
                vs = vs % 100;
                if (vs >=20) vs = vs % 10;
                if (vs >=2 && vs <=4) unit_type =1;
                else unit_type=2;
            }
        }
    }
        
    c=buf->inptr;
    while (*c && microlena_isspace(*c)) c++;
    
    
    char *d=microlena_find_unit(c, &c, NULL);
    if (d) {
        if (push_unit(buf,d,unit_type) < 0) return -1;
        buf->inptr = c;
    }
    return 1;
}

static void bof(struct microlena_Buffer *buf)
{
    while (*buf->inptr) {
        if (strchr(zbeg,*buf->inptr)) break;
        if (*buf->inptr == '-' && buf->inptr[1] && microlena_isdigit(buf->inptr[1])) break;
        if (microlena_isalnum(*buf->inptr)) break;
        buf->inptr++;
    }
}

int microlena_eoph(const char *c,const char **cc)
{
    int typ = 0;
    if (*c == '-' && microlena_isalnum(c[1])) return 0;
    for (;*c;c++) {
        if (microlena_isalnum(*c)) break;
        if (microlena_isspace(*c)) continue;
        if (*c == '-' && c[1] >= '0' && c[1] <='9') break;
        //if (!typ && *c == '-' && c[1]>='0' && c[1]<='9') return 0;
        if (*c == '.' ) {
            if (typ < PHRA_DOT) typ = PHRA_DOT;
        }
        else if (strchr("(),-",*c)) {
            if (typ < PHRA_COMMA) typ = PHRA_COMMA;
        }
        else if (strchr(":;",*c)) {
            if (typ < PHRA_COLON) typ = PHRA_COLON;
        }
        else if (*c == '!') {
            if (typ < PHRA_EXCLA) typ = PHRA_EXCLA;
        }
        else if (*c == '?') {
            if (typ < PHRA_QUE) typ = PHRA_QUE;
        }
        else {
            break;
        }
    }
    if (!*c && !typ) typ = PHRA_DOT;
    if (cc) *cc=c;
    return typ;
}


static int is_numeric(const char *str)
{
    if (microlena_isdigit(*str)) return 1;
    if (*str == '-' && str[1]>='0' && str[1] <= '9') return 1;
    return 0;
}

static int do_numeric(struct microlena_Buffer *buf)
{
    int rc;
    rc=getIP4(buf);
    if (rc) return rc;
    rc=getHour(buf);
    if (rc) return rc;
    rc=getFixed(buf);
    if (rc) return rc;
    char *c;uint8_t fem;
    int v=strtol(buf->inptr,&c,10);
    while (*c && microlena_isspace(*c)) c++;
    char *d=microlena_find_unit(c, &c, &fem);
    
    if (d) {
        char *dx=fem?"fx":"mx";
        int typ;
        if (microlena_SpeakNumberS(buf, v, &dx)<0) return -1;
        if (v<0) v=-v;
        if (v == 1) typ=0;
        else {
            v=v % 100;
            if (v >=20) v = v % 10;
            if (v >=2 && v <=4) typ=1;
            else typ=2;
        }
        if (push_unit(buf,d,typ) < 0) return -1;
        buf->inptr = c;
        return 1;
    }
    else return microlena_integer(buf, &buf->inptr);
}
static int is_plain_word(const char *str, uint8_t *vow, uint8_t *uc)
{
    int i;
    *uc=1;
    for (i=0,*vow=0;;i++) {
        if (str[i] == '~' && strchr("'!,",str[i+1])) {
            i++;
            continue;
        }
        if (str[i] == '\'' && microlena_isalnum(str[i+1])) continue;
        if (!microlena_isalnum(str[i])) return i;
        if (microlena_isdigit(str[i])) return 0;
        if (microlena_isvowel(str[i])) *vow += 1;
        if (!microlena_isupper(str[i])) *uc=0;
    }
    return 0;
}

static char *spld[]={"a","be","c~'e","de","e","ef","gie","ha",
    "i","jot","ka","el","em","en","o","pe","ku","er","es",
    "te","u","fa\263","vu","iks","igrek","zet"};
static char *sple[]={"\261a zogonkiem","\352e z ogonkiem","\363o z kresk\261",
    "\277\277et","\277\277i","\361ni","\266\266i","\346ci",NULL};

int microlena_SpellChar(struct microlena_Buffer *buf, char c, int *bg)
{

    int j;
    if (*bg == 2) {
        blank(buf);
        *bg=0;
    }
    if (c =='\'') {
        pushstr(buf,"apostrof",1);
        *bg=0;
        return 1;
    }
    
    if (c >= 'a' && c <= 'z') {
        if (!(*bg)) pushstr(buf,"[1]",1);
        else pushstr(buf,"~'",0);
        pushstr(buf,spld[c-'a'],0);
        *bg=1;
        return 1;
    }
    if (c >= '0' && c <= '9') {
        if (microlena_int(buf, c-'0') < 0) return -1;
        *bg=0;
        return 1;
    }
    for (j=0;sple[j];j++) if (sple[j][0] == c) break;
    if (sple[j]) {
        if (strchr(sple[j],' ')) {
            *bg=0;
            pushstr(buf,sple[j]+1,bg ?0 : 1);
        }
        else {
            if (!bg) pushstr(buf,"[1]",1);
            else pushout(buf,'_');
            pushstr(buf,sple[j]+1,0);
            *bg=1;
        }
    }
    else {
        *bg=2;
        pushstr(buf,"litera",1);
    }
    return 1;
}


int microlena_Spell(struct microlena_Buffer *buf, int count)
{
    int i,bg,j;
    for (i=bg=0;i<count;i++) {
        char c=microlena_tolower(*buf->inptr++);
        if (microlena_SpellChar(buf, c,&bg) <0) return -1;
    }
    return 1;
}

static int convert_mixed_word(struct microlena_Buffer *buf)
{
    uint8_t done=0;
    //printf("CMI %s\n",buf->inptr);
    while(*buf->inptr) {
        blank(buf);
        if (microlena_isdigit(*buf->inptr)) {
            if (microlena_integer(buf, &buf->inptr) < 0) return -1;
            done=1;
            continue;
        }
        if (!microlena_isalnum(*buf->inptr)) break;
        char *c;int len,i;
        uint8_t vow=0, uc=1;
        for (c=buf->inptr,len=0;*c;c++,len++) {
            if (!microlena_isalpha(*c)) break;
            if (microlena_isvowel(*c)) vow=1;
            if (!microlena_isupper(*c)) uc=0;
        }
        done =1;
        if (vow && !uc) {
            for (i=0;i<len;i++) pushout(buf, microlena_tolower(buf->inptr[i]));
            buf->inptr=c;
        }
        else {
            if (microlena_Spell(buf,len) <0) return -1;
        }
        
    }
    return done;
}



int microlena_Phraser(struct microlena_Buffer *buf)
{
    int rc,i,nw,first=1;
    microlena_StartTextBuffer(buf);
    bof(buf);
    if (!*buf->inptr) return 0; // tak nie powinno byc
    for (nw=0;;nw++,first=0) {
        //if (!first) {
            rc=microlena_eoph( (const char *)buf->inptr, (const char **)&buf->inptr);
            if (rc) {
                if (!nw) {
                    return 0;
                }
                pushout(buf,0);
                //printf(" BB inbuf=%p inptr=%p inbufptr=%p\n",buf->inbuf, buf->inptr, buf->inbufptr);
                buf->inbufptr = buf->inptr;
                microlena_ShiftBuffer(buf);
                buf->currentPhrase = rc;
                //printf(" B2 inbuf=%p inptr=%p inbufptr=%p\n",buf->inbuf, buf->inptr, buf->inbufptr);
                return rc;
            }
            //bof(buf); // podwójne wywołanie... ale się może przydać
            //if (!*buf->inptr) return -1; // tak nie powinno byc
        //}
        // tutaj frazer
        rc = microlena_match_recognizer(buf);
        //printf("Recognizer returned %d\n",rc);
        if (rc == 0) rc = microlena_match_udict(buf);
        if (rc < 0) return rc;
        if (rc) continue;
        if (strchr(zbeg, *buf->inptr)) {
            for (i=0;znaczki[i];i++) if (*buf->inptr == znaczki[i][0]) break;
            if (znaczki[i]) {
                pushstr(buf,"[z]",1);
                pushstr(buf, znaczki[i]+1,0);
            }
            else pushstr(buf,"[z]znak",1);
            buf->inptr++;
            continue;
        }
        if (is_numeric(buf->inptr)) {
            rc=do_numeric(buf);
            if (rc < 0) return rc;
            else continue;
        }
        uint8_t vow=0, uc=0;
        rc=is_plain_word(buf->inptr, &vow, &uc);
        if (rc) {
            blank(buf);
            if ((*buf->inptr == 'w' || *buf->inptr == 'z') && rc == 1) {
                pushout(buf, microlena_tolower(*buf->inptr++));
            }
            else if (vow && !uc) {
                for (i=0; i<rc; i++) pushout(buf, microlena_tolower(*buf->inptr++));
            }
            else {
                rc=microlena_Spell(buf,rc);
                if (rc < 0) return rc;
            }
            continue;
        }
        if (convert_mixed_word(buf)<0) return -1;
        continue;
    }
    
//    while (buf->inptr) {
//        int rc=getPhraserPart(buf);
//    }
        
    
}
