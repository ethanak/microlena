#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "common.h"

#define MILDIT_MM 0
#define MILDIT_FM 1
#define MILDIT_NM 2
#define MILDIT_MC 3
#define MILDIT_FD 4
#define MILDIT_MD 5
#define MILDIT_FB 6
#define MILDIT_MN 7
#define MILDIT_AP 8
#define MILDIT_PB 9
#define MILDIT_MX 10
#define MILDIT_FX 11
#define MILDIT_F0 12
#define MILDIT_AC 13
#define MILDIT_MT 14
#define MILDIT_XN 15
#define MILDIT_YM 16
#define MILDIT_YF 17
#define MILDIT_SM 18
#define MILDIT_SF 19

const char *mx_uno[]={
        "zero","jeden","dwa","trzy","cztery","pi\352\346","sze\266\346","siedem",
        "osiem","dziewi\352\346","dziesi\352\346","jedena\266cie","dwana\266cie","trzyna\266cie",
        "czterna\266cie","pi\352tna\266cie","szesna\266cie","siedemna\266cie","osiemna\266cie",
        "dziewi\352tna\266cie"};
const char *mx_dec[]={
    NULL, NULL, "dwadzie\266cia","trzydzie\266ci","czterdzie\266ci","pi\352\346dziesi\261t","sze\266\346dziesi\261t",
    "siedemdziesi\261t","osiemdziesi\261t","dziewi\352\346dziesi\261t"};

const char *mx_cen[]={
    NULL, "sto","dwie\266cie","trzysta","czterysta","pi\352\346set","sze\266\346set",
    "siedemset","osiemset","dziewi\352\346set"};

const char *mx_units[][3]={
    {"tysi\261c","tysi\261ce","tysi\352cy"},
    {"milion","miliony","milion\363w"},
    {"miliard","miliardy","miliard\363w"},
    {"bilion","biliony","bilion\363w"}};


int microlena_integer(struct microlena_Buffer *buf, char **inps)
{
    char *inp = *inps;
    if (*inp == '-') {
        pushstr(buf, "minus", 1);
        inp++;
    }
    /*
    if (*inp == '0') {
        while (*inp >='0' && *inp<='9') {
            pushstr(buf, mx_uno[(int)(*inp++) - '0'],1);
        }
        *inps=inp;
        return 1;
    }
    */
    int unit = 0;
    int blen = 0;
    int form;
    for (blen=0; inp[blen]>='0' && inp[blen] <='9';blen++);
    if (blen > 12) {
        while (*inp >='0' && *inp<='9') {
            pushstr(buf, mx_uno[(int)(*inp++) - '0'],1);
        }
        *inps=inp;
        return 1;
    }
    while (*inp == '0') {
        pushstr(buf,"zero",1);
        inp++;
        blen--;
    }
    if (!*inp && !microlena_isalnum(*inp)) {
        *inps=inp;
        return 1;
        
    }
    
    //printf("B T %s\n", inp);
    unit = (blen-1) / 3;
    while (blen) {
        int nc=(blen) % 3; if (!nc) nc=3;
        int digit,was=0;
        blen -= nc;
        // jeden traktujemy specjalnie
        if (unit && nc == 1 && *inp == '1') {
            inp++;
            unit--;
            pushstr(buf, mx_units[unit][0],1);
            continue;
        }
        // zero również traktujemy specjalnie
        // ale to już załatwiliśmy przy
        // zerach wiodących

        // setki
        if (nc == 3) {
            digit = *inp++ - '0';
            if (digit) {
                pushstr(buf, mx_cen[digit],1);
                was=2; // znaczy się: mówiliśmy o setkach
            }
            nc--;
        }
        digit = *inp++ - '0';
        if (nc == 2) digit = (10 * digit) + *inp++ - '0';
//      w digit jest już liczba
//        printf("DI=%d\n", digit);
//      zero mieliśmy wcześniej
        if (digit >= 20) {
            pushstr(buf, mx_dec[digit / 10],1);
            digit %= 10;
            was |= 1;
        }
        if (digit > 0) {
            pushstr(buf, mx_uno[digit],1);
            was = 1;
        }
        if (unit && was) {
            if (digit >= 2 && digit <=4) form =1;
            else {
                form = 2;
                if (digit == 1 && was == 1) form = 0;
            }
            pushstr(buf, mx_units[unit-1][form],1);
        }
        unit -= 1;
    }
    *inps=inp;
    return 1;
}

static const char *milena_mons[]={"","stycznia","lutego","marca","kwietnia",
	"maja","czerwca","lipca","sierpnia","wrze\266nia","pa\274dziernika",
	"listopada","grudnia",NULL};

static const char *dec_fin[]={
	"y","a","e","emu","ej","ego","\261","ym","o","ych"};

static const char *jed_por[]={
	"zerow","pierwsz","drugi","trzeci","czwart",
	"pi\261t","sz\363st","si\363dm","\363sm","dziewi\261t",
	"dziesi\261t","jedenast","dwunast","trzynast","czternast",
	"pi\352tnast","szesnast","siedemnast","osiemnast","dziewi\352tnast"};
static const char *dec_por[]={
	NULL,NULL,"dwudziest","trzydziest","czterdziest",
	"pi\352\346dziesi\261t","sze\266\346dziesi\261t","siedemdziesi\261t",
	"osiemdziesi\261t","dziewi\352\346dziesi\261t"};
static const char *dec_pac[]={
	NULL,NULL,"dwudziesto","trzydziesto","czterdziesto",
	"pi\352\346dziesi\352cio","sze\266\346dziesi\352cio","siedemdziesi\352cio",
	"osiemdziesi\352cio","dziewi\352\346dziesi\352cio"};

static const char *hun_por[]={
	NULL,NULL,"dwu","trzech","czterech","pi\352\346","sze\266\346",
		"siedem","osiem","dziewi\352\346"};
static const char *mil_por[]={
	NULL,"jedno","dwu","trzy","cztero","pi\352cio","sze\266cio",
	"siedmio","o\266mio","dziewi\352cio","dziesi\352cio"};

static const char *sgl_prd[]={
	NULL,"jednym","dwoma","trzema","czterema","pi\352cioma","sze\266cioma",
	"siedmioma","o\266mioma","dziewi\352cioma","dziesi\352cioma"};


static const char *hun_pac[]={NULL,"stu","dwustu","trzystu",
	"czterystu","pi\352\346set","sze\266\346set","siedemset","osiemset","dziewi\352\346set"};
static const char *hun_bac[]={NULL,"stu","dwustu","trzystu",
	"czterystu","pi\352ciuset","sze\266ciuset","siedmiuset","o\266miuset","dziewi\352ciuset"};

static const char *dec_ilu[]={
	NULL,NULL,"dwudziestu","trzydziestu","czterdziestu",
	"pi\352\346dziesi\352ciu","sze\266\346dziesi\352ciu","siedemdziesi\352ciu",
	"osiemdziesi\352ciu","dziewi\352\346dziesi\352ciu"};
static const char * const jed_ilu[]={
	NULL,"jeden","dw\363ch","trzech","czterech",
	"pi\352ciu","sze\266ciu","siedmiu","o\266miu","dziewi\352ciu",
	"dziesi\352ciu","jedenastu","dwunastu","trzynastu","czternastu",
	"pi\352tnastu","szesnastu","siedemnastu","osiemnastu","dziewi\352tnastu"};

int microlena_int(struct microlena_Buffer *buf, int n)
{
    char cb[16],*c=cb;
    sprintf(cb,"%d", n);
    return microlena_integer(buf, &c);
}
static int microlena_poilu(struct microlena_Buffer *buf, int n);
static int microlena_koloilu(struct microlena_Buffer *buf, int n, int female);
static int microlena_przedilu(struct microlena_Buffer *buf, int n, int female);

    
static int microlena_koloilu(struct microlena_Buffer *buf, int n, int female)
{
    int tau;
    if (n == 1) {
        if (female) {
            pushstr(buf, "jednej", 1);
        } else {
            pushstr(buf, "jednego", 1);
        }
        return 1;
    }
    if (n < 1 || n > 999999)
        return microlena_int(buf, n);
    if (n < 1000)
        return microlena_poilu(buf,n);
    tau = n / 1000;
    n %= 1000;
    if (tau > 1) {
        if (microlena_poilu(buf, tau) < 0) return -1;
        pushstr(buf, "tysięcy",1);
    } else {
        pushstr(buf, "tysiąca",1);
    }
    if (n) {
        return microlena_poilu(buf, n);
    }
    return 1;
}

    
static int microlena_przedilu(struct microlena_Buffer *buf, int n, int female)
{
    char cbuf[64];
    if (n >= 1000 && n < 1000000 && !(n % 1000)) {
        n /= 1000;
        if (n == 1) {
            pushstr(buf, "tysiącem", 1);
            return 1;
        } else {
            if (n >= 100) {
                if (n % 100 == 0) {
                    strcpy(cbuf, hun_bac[n / 100]);
                    strcpy(cbuf + strlen(cbuf) - 1, "oma");
                    pushstr(buf, cbuf, 1);
                } else {
                    pushstr(buf, hun_bac[n / 100],1);
                }
                n %= 100;
            }
            if (n) {
                if (n == 1) {
                    pushstr(buf, "jeden", 1);
                } else if (n <= 10) {
                    pushstr(buf, sgl_prd[n] ,1);
                } else if (n <= 19) {
                    pushstr(buf,jed_por[n],1);
                    pushstr(buf, "oma",0);
                } else {
                    strcpy(cbuf, dec_ilu[n / 10]);
                    strcpy(cbuf + strlen(cbuf) - 1, "oma");
                    pushstr(buf, cbuf, 1);
                    if (n % 10) {
                        pushstr(buf, sgl_prd[n % 10], 1);
                    }
                }
            }
        }
        pushstr(buf, "tysiącami",1);
        return 1;
    }
    if (n < 1 || n > 999)
        return microlena_int(buf, n);
    if (female) {
        if (n == 1) {
            pushstr(buf, "jedną", 1);
            return 1;
        }
        if (n == 2) {
            pushstr(buf, "dwiema", 1);
            return 1;
        }
    } else if (n == 1) {
        pushstr(buf, "jednym", 1);
        return 1;
    }
    if (n >= 100) {
        int st = n / 100;
        n = n % 100;
        if (!n) {
            strcpy(cbuf, hun_bac[st]);
            strcpy(cbuf + strlen(cbuf) - 1, "oma");
            pushstr(buf, cbuf, 1);
            return 1;
        } else {
            pushstr(buf, hun_bac[st], 1);
        }
    }
    if (n == 1) {
        pushstr(buf, "jeden", 1);
    } else if (n <= 10) {
        pushstr(buf, sgl_prd[n], 1);
    } else if (n <= 19) {
        pushstr(buf, jed_por[n],1);
        pushstr(buf, "oma", 0);
    } else {
        strcpy(cbuf, dec_ilu[n / 10]);
        strcpy(cbuf + strlen(cbuf) - 1, "oma");
        pushstr(buf, cbuf, 1);
        if (n % 10) {
            if ((n % 10) == 1) {
                pushstr(buf, "jeden", 1);
            } else {
                pushstr(buf, sgl_prd[n % 10], 1);
            }
        }
    }
    return 1;
}
    
static int microlena_poilu(struct microlena_Buffer *buf, int n)
{
    if (n < 1 || n > 999)
        return microlena_int(buf, n);
    if (n == 1) {
        pushstr(buf, "jednym", 1);
        return 1;
    }
    if (n >= 100) {
        pushstr(buf, hun_bac[n / 100], 1);
        n %= 100;
        if (!n)
            return 1;
    }
    if (!n)
        return 1;
    if (n < 20) {
        pushstr(buf, jed_ilu[n], 1);
        return 1;
    }
    pushstr(buf, dec_ilu[n / 10], 1);
    n %= 10;
    if (n) {
        pushstr(buf, jed_ilu[n], 1);
    }
    return 1;
}

int microlena_SpeakNumber(struct microlena_Buffer *buf, int n,int mode)
{

	int m;
    
	if (n>999999) return microlena_int(buf,n);
	if (mode == MILDIT_SM) {
		return microlena_przedilu(buf, n, 0);
	}
	if (mode == MILDIT_SF) {
		return microlena_przedilu(buf, n, 1);
	}
	if (mode == MILDIT_XN) {
		return microlena_poilu(buf, n);
	}
	if (mode == MILDIT_YM) {
		return microlena_koloilu(buf, n, 0);
	}
	if (mode == MILDIT_YF) {
		return microlena_koloilu(buf, n, 1);
	}
	if (mode == MILDIT_MT) {
		if (n>=1 && n<=12) pushstr(buf, milena_mons[n], 1);
		return 1;
	}
	if (mode < MILDIT_MX) {
		if (!n) {
			pushstr(buf, jed_por[0], 1);
			pushstr(buf, dec_fin[mode], 0);
			return 1;
		}
		m=n%100;
		if (m) {
			if (n>100) {
				if (microlena_int(buf, n-m)<0) return -1;
			}
			if (m>=20) {
				pushstr(buf, dec_por[m/10], 1);
				pushstr(buf, dec_fin[mode], 0);
				m%=10;
			}
			if (m==2 && mode == MILDIT_FM) {
				pushstr(buf, "druga", 1);
			}
			else if (m==2 && mode == MILDIT_FB) {
				pushstr(buf, "drugą", 1);
			}
			else if (m==2 && mode == MILDIT_AP) {
				pushstr(buf, "drugo", 1);
			}
			else if (m) {
				const char *c=dec_fin[mode];
				pushstr(buf, jed_por[m], 1);
				if ((m==2 || m==3) && *c=='y') c++;
				if (*c) pushstr(buf, c, 0);
			}
			return 1;
		}
		m=n%1000;
		if (m) {
			if (n>1000) {
				microlena_int(buf, n-m);
			}
			m/=100;
			if (m>1) pushstr(buf, hun_por[m], 1);
			pushstr(buf, "setn", 0);
			pushstr(buf, dec_fin[mode],0);
			return 1;
		}
		if (n==1000) {
			if (mode == MILDIT_AP) {
				pushstr(buf, "tysiąc", 1);
			}
			else {
				pushstr(buf, "tysięczn", 1);
				pushstr(buf, dec_fin[mode], 0);
			}
			return 1;
		}
		m=n/1000;
		if (microlena_SpeakNumber(buf, m,MILDIT_AC) < 0) return -1;
		pushstr(buf, "tysięczn", 0);
		pushstr(buf, dec_fin[mode], 0);
		return 1;
	}

	if (!n && (mode == MILDIT_FX || mode == MILDIT_MX)) {
		pushstr(buf, "zero", 1);
		return 1;
	}
	switch(mode) {
		case MILDIT_F0: if (n<10) pushstr(buf, "zero",1);
		case MILDIT_FX:
            if (mode == MILDIT_FX && n == 1) {
                pushstr(buf, "jedna",1);
                return 1;
            }
			m=n%100;
			if (m==12 || m%10!=2) return microlena_int(buf, n);
			if (m!=2) {
				if (microlena_int(buf, n-2) < 0) return -1;
			}
			pushstr(buf, "dwie", 1);
			return 1;
		default: break;
	}
	if (mode == MILDIT_AC) {
		m=n%1000;
		if (!n) {
			pushstr(buf, "zero", 1);
			return 1;
		}
		if (m) {
			if (n>1000) {
				if (microlena_int(buf, n-m) < 0) return -1;
			}
			if (!m%100) {
				pushstr(buf, hun_bac[m/100],1);
				return 1;
			}
			if (m>=100) pushstr(buf, hun_pac[m/100],1);
			m%=100;
			if (!m) return 1;
			if (m<=10) {
				pushstr(buf, mil_por[m], 1);
				return 1;
			}
			if (m<20) {
				pushstr(buf, jed_por[m], 1);
				pushout(buf, 'o');
				return 1;
			}
			pushstr(buf, dec_pac[m/10], 1);
			m%=10;
			if (m) pushstr(buf, mil_por[m], 1);
			return 1;
		}
		return microlena_int(buf, n);
	}
	return microlena_int(buf, n);
}

static int get_mildit_type(const char **format)
{
	int ftyp=MILDIT_MX,j;
    static const char *mr_mildit="mmfmnmmcfdmdfbmnappbmxfxf0acmtxnymyfsmsf";
	for (j=0;mr_mildit[2*j];j++) if (**format==mr_mildit[2*j] && (*format)[1]==mr_mildit[2*j+1]) {
		*format += 2;
        return j;
	}
	return MILDIT_MX;
}

int microlena_SpeakNumberS(struct microlena_Buffer *buf, int n,const char **fmt)
{
    int typ = get_mildit_type(fmt);
    return microlena_SpeakNumber(buf, n,typ);
}

