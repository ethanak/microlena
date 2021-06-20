#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"

#define despace(buf) while (*buf->inptr && microlena_isspace(*buf->inptr)) buf->inptr++

struct wordData {
    uint16_t flags;
    uint8_t stress;
    uint8_t stressp;
    char word[64];
};

static int getPreWord(struct microlena_Buffer *buf, struct wordData *wd)
{
    wd->flags = 0;
    wd->stress = 0;
    wd->stressp = 0;
    int i;
    while (*buf->inptr) {
        despace(buf);
        if (*buf->inptr != '[') break;
        buf->inptr++;
        wd->flags |= MILD_FOUND;
        while (*buf->inptr) {
            if (*buf->inptr == ']') {
                buf->inptr++;break;
            }
            if (*buf->inptr == '+' && buf->inptr[1] >= '1' && buf->inptr[1] <= '4') {
                wd->stress = buf->inptr[1] - '0';
                buf->inptr += 2;
                continue;
            }
            if (*buf->inptr == '-' && buf->inptr[1] >= '1' && buf->inptr[1] <= '4') {
                wd->stressp = buf->inptr[1] - '0';
                buf->inptr += 2;
                continue;
            }
            char cc;
            switch (cc=*buf->inptr++) {
                case '1' ... '4': wd->stress = cc-'0';break;
                case 'k': wd->flags |= MILD_K;break;
                case 'U': wd->flags |= MILD_U | MILD_COND;break;
                case 'v': wd->flags |= MILD_VERB;break;
                case 'z': wd->flags |= MILD_FOUND;break;
                case 'C': wd->flags |= MILD_COND;break;
                case 'p': wd->flags |= MILD_P;break;
                case 'r': wd->flags |= MILD_R;break;
                case 'i':
                case 's': wd->flags |= MILD_SEP | MILD_U;break;
                case 'b': wd->flags |= MILD_BREAK;break;
            }
        }
    }
    if (!*buf->inptr) {
        wd->flags = MILD_NOWORD;
        return 0; // not a word?
    }
    for (i=0;*buf->inptr;i++,buf->inptr++) {
        if (strchr("'~,!'_", *buf->inptr)) wd->flags |= MILD_FOUND;
        if (microlena_isalnum(*buf->inptr) || strchr("'~,!'_@", *buf->inptr)) {
            if (i>62) return -1;
            wd->word[i]=*buf->inptr;
        }
        else break;
    }
    wd->word[i]=0;
    if (!wd->flags && !wd->stress && !wd->stressp) {
        wd->flags |= microlena_WordClass(wd->word, &wd->stress, &wd->stressp);
    }
    return 1;
}

#define CER(n,a,b) wordData[n].flags = (wordData[n].flags & ~(a)) | (b)

int microlena_PreStress(struct microlena_Buffer *buf)
{
    struct wordData wordData[3];
    int nword;
    
    int n1=getPreWord(buf, &wordData[0]),n2;
    if (n1 <= 0) return n1;
    n2=getPreWord(buf,&wordData[1]);
    if (n2<0) return n2;
    
    //if ((n2=getPreWord(buf,&wordData[1]))<0) return -1;
    if (getPreWord(buf,&wordData[2])<0) return -1;

    buf->outptr=buf->buffer;
    pushout(buf,buf->currentPhrase + '0');
    pushout(buf,':');
    for (nword=0;;nword++) {
        if (wordData[0].flags & MILD_NOWORD) break;
        // unstress operator
        if (wordData[0].flags & MILD_OPER) {
            if (wordData[1].flags & MILD_VERB) {
                wordData[0].flags &= ~(MILD_OPER);
                wordData[0].flags |= ~(MILD_U);
            }
        }
        if ((wordData[0].flags & MILD_P) &&
                    (wordData[1].flags & MILD_R)) {
            if ((wordData[1].flags & MILD_COND) && (wordData[2].flags & MILD_NOWORD)) {
                CER(0,MILD_P | MILD_COND, MILD_U);
                CER(1,MILD_R | MILD_COND | MILD_U,MILD_FOUND);
            }
            else {
                CER(0,MILD_P | MILD_COND | MILD_U, MILD_FOUND);
                CER(1,MILD_R | MILD_COND, MILD_U | MILD_NOUNSTRESS);
            }
            
        }
        if (wordData[0].flags & (MILD_P | MILD_R)) {
            CER(0, MILD_P | MILD_R, MILD_U);
        }
        if (wordData[1].flags & MILD_NOWORD) { //eoph
            if ((wordData[0].flags & (MILD_U | MILD_NOUNSTRESS)) == MILD_U &&
                (wordData[0].flags&& (wordData[1].flags & MILD_NOWORD))) {
            CER(0, MILD_U, MILD_FOUND);
            }
        }
        if ((wordData[0].flags & MILD_K) && !(wordData[1].flags & (MILD_VERB | MILD_PPAS))) {
            CER(0, MILD_K, MILD_FOUND);
        }
        
        // tutaj jeszcze dla COND ale to potem;
        blank(buf);
        
        if ((wordData[0].flags & (MILD_BREAK | MILD_K | MILD_U | MILD_VERB | MILD_SEP)) || wordData[0].stress || wordData[0].stressp) {
            pushout(buf,'[');
            if (wordData[0].stress) pushout(buf,wordData[0].stress+'0');
            if (wordData[0].stressp) {
                pushout(buf,'-');
                pushout(buf,wordData[0].stressp+'0');
            }
            //if (flags & MILD_COND) pushout('C');
            if (wordData[0].flags & MILD_VERB) pushout(buf,'v');
            if (wordData[0].flags & MILD_U) pushout(buf,'u');
            if (wordData[0].flags & MILD_SEP) pushout(buf,'s');
            if (wordData[0].flags & MILD_K) pushout(buf,'k');
            if (wordData[0].flags & MILD_BREAK) pushout(buf,'b');
            pushout(buf,']');
        }
        pushstr(buf,wordData[0].word,0);
        if (wordData[1].flags & MILD_NOWORD) break;
        wordData[0] = wordData[1];
        wordData[1] = wordData[2];
        if (!(wordData[1].flags & MILD_NOWORD)) {
            if (getPreWord(buf, &wordData[2]) < 0) return -1;
        }
    } // for nword
    if (*buf->inptr == ';') {
        buf->inptr++;
        buf->buffer[0] = '0' + PHRA_COMMA;
    }
    pushout(buf, 0);
    microlena_CorrectPrestresser(buf);
    return nword+1;
}
