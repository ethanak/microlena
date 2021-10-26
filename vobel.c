#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "milv_datatypes.h"
#include "config.h"

#ifdef LITE_DATA
#include "lite_data.h"
#else
#include "full_data.h"
#endif
#include "common.h"

#define UDIF_ATEND 1
#define UDIF_ABBR 2
#define UDIF_UNSTR 4
#define UDIF_COPY 8

#define STR(a) ((char *)wordarray+(a))

static int eqc(int a, int b)
{
    if (!microlena_isalnum(b)) return a == b;
    if (microlena_tolower(b) != b) {
        return a == b;
    }
    return microlena_tolower(a) == b;
}

static int eow(const char *c)
{
    //printf("EOW <%s>\n",c);
    if (!*c) return 1;
    return (*c == '\'' || microlena_isalnum(*c)) ? 0 : 1;
    
}

static struct{
    const char *beg;
    const char *end;
} udic_param[8];
static const char * const *user_units;
static const char * const *user_udict;

void microlena_setUserDict(const char * const *units, const char * const *dict)
{
    user_units = units;
    user_udict = dict;
}
static const char *match_pattern_udict(const char *c, const char *d, int flags, int *npar)
{
    *npar=0;
    while (*d) {
        if (*d == '_') {
            while (*c && microlena_isspace(*c)) c++;
            d++;
            continue;
        }
        if (*d == '+') {
            if (!(microlena_isspace(*c))) break;
            while (*c && microlena_isspace(*c)) c++;
            d++;
            continue;
        }
        if (*d == '`') {
            while (*c && microlena_isspace(*c)) c++;
            if (*c == '\'' || *c =='`') {
                c++;
                while (*c && microlena_isspace(*c)) c++;
            }
            d++;
            continue;
        }
        if (*d == '\'') {
            if (*c != '\'' && *c !='`') {
                break;
            }
            c++;
            while (*c && microlena_isspace(*c)) c++;
            d++;
            continue;
        }
        if (*d == '~') {
            if (d[1] == '~') {
                while (*c && (*c == ' ' || microlena_isspace(*c))) c++;
                d+=2;
                continue;
            }
            if (*c++!='-') break;
            while (*c && microlena_isspace(*c)) c++;
            d++;
            continue;
        }
        if (*d == '[') {
            d++;
            while (*d && *d!=']') {
                if (*d == *c) break;
                d++;
            }
            if (!*d || *d == ']') break;
            while (*d && *d!=']') d++;
            if (!*d) break;
            d++;
            continue;
        }
        if (*d == '\\') {
            d++;
            if (*c == *d) c++;
            d++;
            continue;
        }
        if (*d=='(') {
            const char *e;
            uint8_t found=0;
            for (;;) {
                e=c;
                d++;
                while (*d && *d!='|' && *d!=')') {
                    if (*e != *d) break;
                    d++;e++;
                }
                if (!*d) return 0;
                if (*d==')' || *d=='|') {
                    found=1;
                    break;
                }
                while (*d && *d!=')' && *d != '|') d++;
                if (*d != '|') break;
            }
            if (!found) break;
            while (*d && *d!=')') d++;
            if (!*d) break;
            if (*npar < 8) {
                udic_param[*npar].beg = c;
                udic_param[(*npar)++].end = e;
            }
            c=e;
            d++;
            continue;
        }
            
        if (*d=='#') {
            d++;
            int nmac = *d - 'A';
            d=udict_macros[nmac];
            const char *e;
            uint8_t found=0;
            for (;;d++) {
                e=c;
                while (*d && *d!='|') {
                    if (*e != *d) break;
                    d++;e++;
                }
                if (!*d || *d=='|') {
                    
                    found=1;
                    break;
                }
                while (*d && *d != '|') d++;
                if (!*d) {
                    break;
                }
            }
            if (!found) break;
            d="";
            if (*npar < 8) {
                udic_param[*npar].beg = c;
                udic_param[(*npar)++].end = e;
            }
            c=e;
            break;
            
        }
        if (!eqc(*c, *d)) {
            break;
        }
        c++;
        d++;
    }
    if (*d) return NULL;
    if (!eow(c)) return NULL;
    if (flags & UDIF_ATEND) {
        if (!microlena_eoph(c, NULL)) return NULL;
    }
    return c;
}

int microlena_match_udict(struct microlena_Buffer *buf)
{
    int i,npar, flags, stress;
    const char *c, *d, *value = NULL; uint8_t found = 0;
    if(user_udict) for (i=0;user_udict[i]; i++) {
        c=buf->inptr;
        npar=0;
        d=user_udict[i];
        flags = *d++;
        if (!(c=match_pattern_udict(c, d, flags, &npar))) continue;
        while (*d++);
        value = d;
        stress = (flags >> 4) & 7;
        found = 1;
        break;
    }
    if (!found) for (i=udict_count-1;i>=0;i--) {
        c=buf->inptr;
        npar=0;
    
        d=STR(udict_data[i].pattern_offset);
        if (!(c=match_pattern_udict(c, d, udict_data[i].flags, &npar))) continue;
        value = STR(udict_data[i].value_offset);
        flags = udict_data[i].flags;
        stress = udict_data[i].stress;
        found = 1;
        break;
    }
    if (!found) return 0;
    if (!(flags & UDIF_ABBR) && stress) {
        char bf[8];
        sprintf(bf,"[%d]",stress);
        pushstr(buf,bf,1);
    }
    else blank(buf);
    if (flags & UDIF_ABBR) {
        if (microlena_Spell(buf, c-buf->inptr)<0) return -1;
    }
    else if (flags & UDIF_COPY) {
        while (buf->inptr < c) {
            *buf->outptr++ = microlena_tolower(*buf->inptr++);
        }
    }
    else {
        int nr = 0;
        const char *ce=value;
        for (;*ce;ce++) {
            if (*ce != '%') pushout(buf,*ce);
            else {
                if (ce[1] >= '1' && ce[1] <='8') {
                    nr = ce[1] - '1';
                    ce++;
                }
                if (nr < npar) {
                    const char *de = udic_param[nr].beg;
                    for (;de < udic_param[nr].end;de++) pushout(buf, *de);
                    nr++;
                }
            }
        }
    }
    buf->inptr=(char *)c;
    return 1;
}

#define TYP_VERB 1
#define TYP_OPERATOR 2
#define TYP_SEPARATOR 3
#define TYP_P 4
#define TYP_R 5
#define TYP_PR 6

 
#define MILDIN_U 0x1
#define MILDIN_COND 0x2
// k - nie łączone z następnym czasownikiem
#define MILDIN_K 0x4
// x - kasowanie operatora przed słowem
#define MILDIN_X 0x8

int reverse2(char *dst, const char *src)
{
    int d=strlen(src);
    int i;
    for (i=0;i<d;i++) dst[d-i-1]=src[i];
    dst[d]=0;
    return d;
}

static int matchstr(const char *word, const char *patr)
{
    int i;
    for (i=0;patr[i];i++) {
        if (patr[i] == '*') break;
        if (!word[i]) return 0;
        if (patr[i] == word[i]) continue;
        if ((patr[i] == 'A'  || patr[i] == '@') && microlena_isvowel(word[i])) continue;
        return 0;
    }
    return i;
}
int microlena_WordClass(char *word, uint8_t *stress, uint8_t *stressp)
{
    int lo, hi, mid,n,flags,i;
    char revbuf[32];
    int dl=strlen(word); if (dl > 31) return 0;
    *stress=0;
    static const uint16_t f2t[]={
        MILD_FOUND,MILD_VERB, MILD_VERB | MILD_OPER, MILD_SEP | MILD_U,
            MILD_P, MILD_R, MILD_P | MILD_R, MILD_FOUND};
    // czy słowo zawiera znaki sterujące lub apostrof?
    if (strpbrk(word,"~'!,")) {
        if (strstr(word, "~!") || strstr(word,"~,")) return MILD_STRE;
        return MILD_FOUND;
    }
    lo=0;
    hi=locvdic_size-1;
    //for (mid=0; mid < hi; mid++) printf("%d %s\n", mid,STR(locvdic[mid].offset));
    //exit(0);
    while (lo <= hi) {
        mid = (lo+hi)/2;
        const char *c=STR(locvdic[mid].offset);
        //printf("Compare %s %s\n",word,c);
        n=strcmp(word,c);
        if (n < 0) hi=mid-1;
        else if (n>0) lo=mid+1;
        else break;
    }
    if (lo <= hi) {
        *stress=locvdic[mid].stress;
        flags = f2t[locvdic[mid].typ];
        
        
        if (locvdic[mid].flags & MILDIN_COND) flags |= MILD_COND;
        if (locvdic[mid].flags & MILDIN_U) flags |= MILD_U;
        if (locvdic[mid].flags & MILDIN_K) flags |= MILD_K;
        if (locvdic[mid].flags & MILDIN_X) flags |= MILD_NOOP;
        return flags;
    }
    if (dl < 3) return 0;
    int csyl;const char *cs;
    for (csyl=0, cs=word;*cs;cs++) {
        if (*cs == 'i' && cs[1] && microlena_isvowel(cs[1])) continue;
        if (microlena_isvowel(*cs)) csyl += 1;
    }
    if (!csyl) return 0;
    reverse2(revbuf,word);
    if (csyl == 1 || dl<=4) {
        if (revbuf[0] == 'c' && revbuf[1] == '\261') return MILD_PPAS;
        lo=0;hi=shortverb_size-1;
        while (lo <= hi) {
            mid=(lo+hi)/2;
            const char *c=STR(shortverbs[mid]);
            n=strcmp(word,c);
            //printf("Compare %s %s\n",word,c);
            if (n < 0) hi=mid-1;
            else if (n>0) lo=mid+1;
            else break;
        }
        if (lo <= hi) {
            return MILD_VERB;
        }
    }
    // sfx1 - czyli końcówki sprawdzane jako pierwsze
    for (i=0; i<sfx1_count; i++) {
        if (matchstr(revbuf, sfx1_data[i])) {
            *stress = sfx1_stress[i];
            return MILD_FOUND;
        }
    }
    // teraz suffixin
    for (i=0; i< suffixin_count;i++) {
        if (matchstr(revbuf,STR(suffixin_data[i].offset))) {
            *stress = suffixin_data[i].stress;
            return (suffixin_data[i].flags &1) ?MILD_VERB : MILD_FOUND;
        }
    }

    // teraz wielki słownik końcówek
    flags = 0;
#ifndef LITE_DATA
    for (i=0; i< suffix_count; i++) {
        int j;
        const char *c=STR(vsuffix[i].suffix_offset);
        int n=matchstr(revbuf,c);
        if (!n) continue;
        int t=vsuffix[i].vob_len & 0x800;
        int len = vsuffix[i].vob_len & 0x7ff;
        if (!len) {
            flags = t ? MILD_VERB : MILD_FOUND;
            break;
        }
        char *ced = revbuf+n;
        const char *ds= c+strlen(c)+1;
        for (j = 0; j<len; j++) {
            if (matchstr(ced, ds)) {
                flags = t ? MILD_VERB : MILD_FOUND;
                break;
            }
            ds += strlen(ds)+1;
        }
        if (j >= len) flags = t ? MILD_FOUND : MILD_VERB;
        break;
    }
    if (flags & MILD_VERB) { // vsfx
        for (i=0; i< vsfx_count;i++) if (matchstr(revbuf,vsfx_data[i])) {
            *stress = vsfx_stress[i];
            break;
        }
    }
    else {
#endif
        //int n=0;
        static const char const *fika[] = {
            "ak","ik","ec","\352k","\261k","ok","mok","hcak"};
        for (i=0; i< 7; i++) if ((n=matchstr(revbuf, fika[i]))) {
            break;
        }
        if (n) {
            char *cd = revbuf + n;
            for (i=0; i<ikactlin_count; i++) {
                n=matchstr(cd, STR(ikactl_data[i].offset));
                if (!n) continue;
                if (ikactl_data[i].flags & 1) {
                    if (!microlena_isvowel(cd[n])) continue;
                }
                if (ikactl_data[i].flags & 2) {
                    if (cd[n]) continue;
                }
                *stress = ikactl_data[i].stress;
                break;
            }
        }
        // i prefiksy
        for (i=0; i<pfx1_count; i++) {
            if (matchstr(word, pfx1_data[i])) {
                *stressp=pfx1_stress[i];
                break;
            }
        }
        if (i >= pfx1_count) {
            for (i=0; i<pfx2_count; i++) {
                if (matchstr(word, pfx2_data[i])) {
                    *stressp=pfx2_stress[i];
                    break;
                }
            }
        }
        // tu ewentualnie liczebniki
        if (i >= pfx2_count) {
            if (matchstr(word, "dziewi\352ci") || matchstr(word,"dziesi\352ci")) *stressp=2;
        }
#ifndef LITE_DATA        
    }
#endif
    return flags;
}

char *microlena_LocString(unsigned int pos)
{
    return (char *)wordarray + pos;
}

void microlena_CorrectPrestresser(struct microlena_Buffer *buf)
{
    char *c,*d,til;
    for (c=buf->buffer;*c;c++) {
        if (*c=='~') til=1;
        else {
            if (!til && *c == '\'') *c='&';
            til=0;
        }
        if ((d=strchr(pho_repstr,*c))) *c = pho_replaceby[d-pho_repstr][0];
    }
}

/* recognizer */
#define MAX_RECOG 8
struct rcg {
    uint8_t name;
    uint8_t type;
    uint8_t len;
    uint8_t intmode;
    char *start;
    char *val;
};

static int recog_expr(char *str, char **ostr, int nexpr)
{
    char *expr = STR(rcgexpr[nexpr]);
    //printf("EXPR IS [%s] [%s]\n", expr,str);
    char *c;
    int found=0;
    for (;;) {
      //      printf("Matching %s\n", expr);
        for (c=str;;) {
        //    printf("Compare %c %c\n",*c, *expr);
            if (*expr == '|')  {
                found=1;
                break;
            }
            if (!*c) {
                if (!*expr) {
                    found=1;
                    break;
                }
                break;
            }
            if (microlena_tolower(*c) != *expr) break;
            c++;
            expr++;
        }
        if (found) {
          //  printf("Matched, next str is [%s]\n",c);
            *ostr=c;
            return 1;
        }
        while (*expr && *expr != '|') expr++;
        if (!*expr) return 0;
        expr++;
        c=str;
    }
    return 0;
}

static char *choice_match(char *str, char *pat)
{
//    printf("Choice match %s to %s\n",pat, str);
    while (*pat) {
        if (!eqc(*str & 255, *pat & 255)) return 0;
        str++;
        pat++;
    }
//    printf("Matched\n");
    return str;
}

static char *recog_choice(char *str, char **ostr, int choice)
{

    int i;
    for (i=0;i<choicerec[choice].table_len; i++) {
        char *c;
        if ((c=choice_match(str,STR(choicetable[choicerec[choice].table_offset+i].pattern_offset)))) {
            *ostr=c;
            return STR(choicetable[choicerec[choice].table_offset+i].value_offset);
        }
    }
    return NULL;
}

static int roman_value(char c)
{
    static const char *roman="IVXLCDM";
    static const int16_t roman_values[]={1,5,10,50,100,500,1000};
    char *s=strchr(roman, c);
    if (!s) return 0;
    return roman_values[s - roman];
}
static int microlena_roman(char *str, char **ostr)
{
    
    int res = 0,s1,s2;
    for (;*str && (s1=roman_value(*str));str++) {
        if (str[1] && (s2=roman_value(str[1]))) {
            if (s1 >= s2)
            {
                res = res + s1;
            }
            else
            {
                res = res + s2 - s1;
                str++;
            }
        }
        else {
            res = res + s1;
        }
    }
    if (res && ostr) *ostr=str;
    return res;
}

static int match_frest_final(char *str, const char *frester)
{
    while (*frester && *str) {
        if (*frester == '[') {
            frester++;
            while (*frester && *frester != ']') {
                if (microlena_tolower(*str) == *frester) break;
                frester++;
            }
            if (!*frester || *frester == ']') return 0;
            while (*frester && *frester != ']') frester++;
            if (!*frester) return 0;
            str++;
            frester++;
            continue;
        }
        if (*frester == '@') {
            if (!microlena_isvowel(*str)) return 0;
            str++;
            frester++;
            continue;
        }
        if (*frester != '(') {
            if (*str++ != *frester++) return 0;
            continue;
        }
        frester++;
        char *c;
        for (;;) {
            c=str;
            while (*frester && *frester != ')' && *frester != '|') {
                if (microlena_tolower(*c) != *frester) break;
                c++;
                frester++;
            }
            if (!*frester) return 0; // babol
            if (*frester == ')' || *frester == '|') { // found
                while (*frester && *frester != ')') frester++;
                if (*frester++ != ')') return 0;
                str=c;
                break;
            }
            // not found
            while (*frester && *frester != ')' && *frester != '|') frester++;
            if (*frester++ != '|') return 0;
            frester++;
            continue;
        }
    }
    if (*frester) return 0;
    return eow(str);
}

static int match_frest(char *str, int frest)
{
    const char *frester=frest_data[frest]+3;
    if (!*str || !microlena_isspace(*str)) return 0;
    str++;
    while (*str && (microlena_isalnum(*str) || *str=='\'')) {
        if (match_frest_final(str,frester)) return 1;
        str++;
    }
    return 0;
}

static int intmode(int v)
{
    int typ;
    if (v < 0) v=-v;
    if (v == 1) {
        typ = 0;
    }
    else {
        v=v % 100;
        if (v >=20) v %=10;
        if (v >=2 && v <=4) typ=1;
        else typ=2;
    }
    return typ;
}

static char *find_recog_unit(char *str, char **ostr, uint8_t female)
{
    int i;
    if (female) female=1;
    for (i=0; i<unit_count; i++) {
        if (units_data[i].fem!=female) continue;
        char *c=str, *d=STR(units_data[i].name_offset);
        for (;*d;d++) if (*c++!=*d) break;
        if (*d) continue;
        if (eow(c)) {
            *ostr=c;
            return STR(units_data[i].value_offset);
        }
    }
    return NULL;
}
            
static int microlena_recognize_pattern(struct microlena_Buffer *buf, char *pattern, struct rcg *recog, uint8_t *nrcg, char **ostr)
{
    char *str = buf->inptr;
    int rcg=0;
    //if (strchr(pattern,'U'))
    for (;*pattern && *str;) {
        if (*pattern == '\\') {
            pattern++;
            if (*str == *pattern) str++;
            pattern++;
            continue;
        }
        if (*pattern == ' ') {
            if (!microlena_isspace(*str)) {
                return 0;
            }
            while (*str && microlena_isspace(*str)) str++;
            while (*pattern && microlena_isspace(*pattern)) pattern++;
            continue;
        }
        if (microlena_isupper(*pattern)) {
            if (*str++ != *pattern++) return 0;
            continue;
        }
        if (*pattern != '{') {
            if (microlena_tolower(*str++) != *pattern++) return 0;
            continue;
        }
        pattern++;
        if (*pattern == ':') {
            int frest=strtol(pattern+1, NULL,10);
            if (ostr) *ostr=str;
            return match_frest(str, frest);
            
        }
        if (!isalnum((int)(*pattern))) return 0; // błędny wzorzec?
        if (rcg >= MAX_RECOG) return 0; // limit!
        recog[rcg].name = (*pattern++) & 255;
        recog[rcg].start = str;
        if (*pattern++ != ':') return 0; // błąd?
        int plen=0;
        if (isdigit((int)(*pattern))) plen = *pattern++ - '0';
        recog[rcg].type = *pattern;
        if (*pattern == 'e') { // expr
            pattern++;
            int enr = strtol(pattern, &pattern,10);
            if (*pattern++ != '}') return 0; // błąd
//            printf("EXPR %d %s\n", enr, pattern);
            if (recog_expr(str, &str, enr)) {
                recog[rcg++].len = str - buf->inptr;
//                printf("Found at %d:<%s>\n",recog[rcg-1].len, recog[rcg-1].start);
                //printf("Trying <%s> <%s>\n",str, pattern);
                continue;
            }
            return 0;
        }
        if (*pattern == 'x') {
            int i;
            pattern++;
            for (i=0; str[i]; i++) if (!isxdigit((int)(str[i]))) break;
            if (plen && i != plen) return 0;
            recog[rcg++].len = i;
            str += i;
            if (*pattern++ != '}') return 0;
            continue;
        }
        if (*pattern == 'f'){
            pattern++;
            if (*str == '-' && microlena_isdigit(str[1])) str++;
            while (*str && microlena_isdigit(*str)) str++;
            if (str == recog[rcg].start) return 0;
            if ((*str == '.' || *str == ',') && microlena_isdigit(str[1])) {
                str+=2;
                while (*str && microlena_isdigit(*str)) str++;
            }
            recog[rcg].len=str-recog[rcg].start;
            recog[rcg].intmode=3;
            rcg++;
            if (*pattern++ != '}') return 0;
            continue;
        }
        if (*pattern == 'd') {
            int i,v1,v2;
            pattern++;
            for (i=0;str[i];i++) if (!microlena_isdigit(str[i])) break;
            if (!i) break;
//            printf("Digita %d, real %d, str <%s>\n", plen, i, str);
            if (plen && i != plen) return 0;
//            printf("Apat <%s>\n", pattern);
            int n = strtol(str,NULL, 10);
            recog[rcg].intmode = intmode(n);
            if (microlena_isdigit(*pattern)) {
                v1=strtol(pattern, &pattern, 10);
//                printf("V1=%d\n",v1);
                if (*pattern++ != '-') continue;
                if (!microlena_isdigit(*pattern)) return 0;
                v2=strtol(pattern, &pattern, 10);
//                printf("V2=%d\n",v2);
                if (n < v1 || n > v2) return 0;
            }
            recog[rcg++].len = i;
            str += i;
//            printf("Afton <%s> <%s>\n", pattern, str);
            if (*pattern++ != '}') return 0;
            continue;
        }
        if (*pattern == 'r') {
            int n,v1,v2;
            pattern++;
            n=microlena_roman(str, &str);
            if (!n) return 0;
            recog[rcg].intmode = intmode(n);
            if (microlena_isdigit(*pattern)) {
                v1=strtol(pattern, &pattern, 10);
//                printf("V1=%d\n",v1);
                if (*pattern++ != '-') continue;
                if (!microlena_isdigit(*pattern)) return 0;
                v2=strtol(pattern, &pattern, 10);
//                printf("V2=%d\n",v2);
                if (n < v1 || n > v2) return 0;
            }
            recog[rcg].len = str-recog[rcg].start;
            rcg++;
            if (*pattern++ != '}') return 0;
            continue;
        }
        if (*pattern == 'c') { //choice
            pattern++;
            int cnr = strtol(pattern, &pattern, 10);
            if (*pattern++ != '}') return 0; // błąd
            char *cs=recog_choice(str,&str,cnr);
            if (!cs) return 0;
            recog[rcg].len = str - recog[rcg].start;
            recog[rcg++].val = cs;
            continue;
        }
        if (*pattern == 'U' || *pattern == 'F') {
            if (!(recog[rcg].val=find_recog_unit(str, &str, *pattern == 'F'))) return 0;
            pattern++;
            
            if (!*pattern || ! ((*pattern >= 'a' && *pattern <='z') || (*pattern >='A' && *pattern <= 'Z'))) return 0;
            recog[rcg].intmode=*pattern++;
            if (*pattern++ != '}') return 0;
            recog[rcg].len = str - recog[rcg].start;
            rcg++;
            continue;
        }
            
        return 0;
    }
    if (!*pattern) {
        *nrcg=rcg;
        *ostr=str;
        return 1;
    }
    return 0;
}

char *microlena_find_unit(char *str, char **ostr, uint8_t *female)
{
    int i;
    if (user_units) for (i=0; user_units[i];i++) {
        uint8_t femm = (user_units[i][0] == 'f') ? 1 : 0;
        char *c=str;
        const char *d=user_units[i]+1;
        for (;*d;d++) if (*c++!=*d) break;
        if (*d) continue;
        if (eow(c)) {
            *ostr=c;
            if (female) *female=femm;
            return (char *)d+1;
        }
    }
    for (i=0; i<unit_count; i++) {
        char *c=str, *d=STR(units_data[i].name_offset);
        for (;*d;d++) if (*c++!=*d) break;
        if (*d) continue;
        if (eow(c)) {
            *ostr=c;
            if (female) *female=units_data[i].fem;
            return STR(units_data[i].value_offset);
        }
    }
    return NULL;
}

static int microlena_push_unit_sa(struct microlena_Buffer *buf, char *unit, struct rcg *recog)
{
    int i,j,typ,flo=0;
    for (i=0;i<unit_count;i++) {
        //printf("%s:%s\n",STR(units_data[i].name_offset), STR(units_data[i].value_offset));
        if (!strcmp(STR(units_data[i].name_offset),unit)) break;
    }
    if (i>=unit_count) return 0;
//    printf("Unit found\n");
    char *c;
    for (j=0;j<recog->len;j++) {
        if (recog->start[j] == '.') {
            flo=1;
            break;
        }
    }
    if (flo) {
        typ=3;
    }
    else {
        int v;
        if (recog->type == 'r') v=microlena_roman(recog->start, NULL);
        else v = strtol(recog->start, NULL, 10);
        typ = intmode(v);
    }
    for (c=STR(units_data[i].value_offset), j=0; j<typ && *c;c++) {
        if (*c=='|') j++;
    }
    if (*c == '|') c++;
    while (*c && *c != '|') pushout(buf, *c++);
//    printf("returning unit OK\n");
    return 1;
}

static int microlena_push_number_sa(struct microlena_Buffer *buf, const char *modf, struct rcg *recog)
{
    if (*modf == 'd') {
        modf++;
        if (strlen(modf) != 2) modf = "mx";
    }
    else modf = "mx";
    char *c;int i, flo=0;
    for (i=0;i<recog->len;i++) if (recog->start[i] == '.') {
        flo=1;
        break;
    }
    if (flo) {
        c=recog->start;
        if (microlena_integer(buf, &c) < 0) return -1;
        if (*c!='.' && *c != ',') return 1;
        c++;
        pushstr(buf,"przecinek",1);
        return microlena_integer(buf, &c);
    }
    if (recog->type == 'r') {
        i=microlena_roman(recog->start,NULL);
    }
    else {
        i=strtol(recog->start,NULL,10);
    }
    return microlena_SpeakNumberS(buf,i,&modf);
}


static int microlena_sayas(struct microlena_Buffer *buf, char *sayas, struct rcg *recog, int rcg, char *ostr)
{
    char *c,modf[32];int i,j;
    blank(buf);
    //printf("SAYAS %s\n",sayas);
    while (*sayas) {
        if (*sayas != '{') {
            pushout(buf, *sayas++);
            continue;
        }
        sayas++;
      //  printf("Searching for name %c\n", *sayas);
        for (i=0; i< rcg; i++) if (recog[i].name == *sayas) break;
        if (i >= rcg) {
//            printf("Not found\n");
            goto eosay;
        }
//        printf("Found %d\n",i); 
        sayas++;
        if (*sayas == ':') sayas++;
        for (c=modf;*sayas && *sayas != '}'; sayas++) {
            if (c < modf+31) *c++=*sayas;
        }
        *c=0;
 //       printf("Recog type %c\n", recog[i].type);
        if (recog[i].type == 'e') {
            for (j=0; j<recog[i].len; j++) {
                pushout(buf,microlena_tolower(recog[i].start[j]));
            }
            goto eosay;
        }
        if (recog[i].type == 'c') {
            pushstr(buf, recog[i].val,0);
            goto eosay;
        }
        if (recog[i].type == 'd' || recog[i].type == 'f' || recog[i].type == 'r') {
//            printf("MODF %s\n",modf);
                
            if (modf[0] == 'u') {
                if (!modf[0]) goto eosay;
                if (microlena_push_unit_sa(buf, modf+1, &recog[i]) < 0) return -1;
                goto eosay;
            }
            if (microlena_push_number_sa(buf,modf,&recog[i]) < 0) return -1;
            goto eosay;
        }
        if (recog[i].type == 'x') { // pomijamy modyfikator
            c=recog[i].start;
            int bg=0;
            for (j=0; j<recog[i].len;j++) {
                microlena_SpellChar(buf, recog[i].start[j],&bg);
            }
            goto eosay;
        }
        if (recog[i].type == 'U' || recog[i].type == 'F') {
            int j;
            for (j=0;j<rcg; j++) if (j != i && (recog[j].name & 255) == recog[i].intmode) break;
            uint8_t typ = 0;
            if (j <rcg) {
                int k;
                for (k=0; k<recog[j].len;k++) if (recog[j].start[k] == '.' || recog[j].start[k] ==',') {
                    typ = 3;
                    break;
                }
                if (!typ) {
                    int v=strtol(recog[j].start, NULL, 10);
                    if (v<0) v=-1;
                    if (v == 1) typ = 3;
                    else typ = 2;
                }
            }
            c=recog[i].val;
            while (typ && *c) {
                if (*c++=='|') typ--;
            }
            while (*c && *c!= '|') {
                pushout(buf,*c++);
            }
            goto eosay;
        }
        pushstr(buf,"sayas",0);
eosay:  
        while (*sayas && *sayas != '}') sayas++;
        if (*sayas) sayas++;
    }
    buf->inptr = ostr;
    return 1;
}

int microlena_match_recognizer(struct microlena_Buffer *buf)
{
    int i,rc;char *ostr;
    struct rcg rcg[8];
    uint8_t nrcg;
    //for (i=recog_count-1; i>=0; i--) {
    for (i=0; i< recog_count;i++) {
        char *pat = STR(rcgrecog[i].offset);
        if ((rc = microlena_recognize_pattern(buf, pat, rcg, &nrcg,&ostr))) {
            if (rc < 0) return rc;
            if (!eow(ostr)) continue;
            // tu może flagi jakie?
            char *sayas = STR(rcgsayas[rcgrecog[i].sayas_pos]);
//            printf("Pattern %s found, sayas %s\n", pat,sayas);
            if (microlena_sayas(buf,sayas, rcg, nrcg, ostr)<0) return -1;
            return rc;
        }
    }
    return 0;
            
        
}
