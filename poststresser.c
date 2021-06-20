#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "config.h"
#define EPN_VOWEL "aeiouyYMOEUI"

struct WordInfo {
    uint8_t flags;
    uint8_t syllables;
    uint8_t prim_stress; // od początku!
    uint8_t sec_stress;  // od początku!
};

#define WINFO_UNSTRES 1
#define WINFO_EXSTRES 2
#define WINFO_KEEP 4
#define WINFO_VERB 8
#define WINFO_SEP 16
#define WINFO_LAST 32
// jeśli poprzedni jest keep a bieżący jest za krótki
//

#define WINFO_STRESSPREV 64
#define WINFO_BRK 128

static char *getWordInfo(char *str, struct WordInfo *w, int prev_keep)
{
    while (*str && microlena_isspace(*str)) str++;
    if (!*str) return 0;
    w->flags = 0;
    w->syllables = 0;
    w->prim_stress = 0;
    w->sec_stress = 0;
    if (*str == '[') {
        str++;
        for (;*str && *str != ']';str++) {
            switch(*str) {
                case 'v': w->flags |= WINFO_VERB; break;
                case 's': w->flags |= WINFO_SEP; break;
                case 'k': w->flags |= WINFO_KEEP; break;
                case 'u': w->flags |= WINFO_UNSTRES; break;
                case 'b': w->flags |= WINFO_BRK; break;
                case '1' ... '4': w->prim_stress = *str - '0';
                case '-': if (str[1] >= '1' && str[1] <= '4') {
                    str++;
                    w->sec_stress = *str - '0';
                
                }
                break;
            }
        }
        if (!*str) return 0;
    }
    for (;*str && !microlena_isspace(*str);str++) {
        if (strchr(EPN_VOWEL, *str)) {
            w->syllables++;
        }
        else if (*str=='~' && (str[1] == ',' || str[1] == '!')) {
            w->flags |= WINFO_EXSTRES;
            str++;
        }
    }
    while (*str && !microlena_isspace(*str)) str++;
    if (!*str) w->flags |= WINFO_LAST;
    if (w->prim_stress == 4 && (w->flags & WINFO_LAST)) {
        w->prim_stress =2;
        w->sec_stress = 4;
    }
    if (!(w->flags & WINFO_EXSTRES)) {
        if (!w->prim_stress) w->prim_stress = 2;
        if (!w->sec_stress) w->sec_stress = 1;
        if (w->prim_stress > w->syllables) {
            if (prev_keep) {
                w->prim_stress -= w->syllables;
                w->flags |= WINFO_STRESSPREV;
                
            }
            else {
                w->prim_stress = 1;
            }
            w->sec_stress = 0;
        }
        else {
            w->prim_stress = w->syllables + 1 - w->prim_stress;
        }
        if (w->sec_stress >= w->prim_stress - 1) w->sec_stress = 0;
        if (w->flags & WINFO_UNSTRES) {
            if (w->syllables == 1) w->sec_stress = 0;
                else w->sec_stress = w->prim_stress;
            w->prim_stress = 0;
        }
    }
    return str;
}

    
int microlena_Poststresser(struct microlena_Buffer *buf)
{
    struct WordInfo winfo[32];
    int word_count,i;
    if (microlena_isdigit(*buf->inptr && buf->inptr[1]==':')) {
        pushout(buf, *buf->inptr++);
        pushout(buf, *buf->inptr++);
    }
    char *str = buf->inptr;
    //subfrazer
    int esyl=0;
    int dsyl=4;
    int last_verb = -1;
    int last_con = -1;
    int mode = 0;
    for (;*str;str++) {
        switch(*str) {
            case '[' : mode = 1; break;
            case ']' : mode = 0; break;
        }
        if (!mode) {
            if (strchr(EPN_VOWEL, *str)) {
                esyl++;
                dsyl++;
            }
            continue;
        }
#ifndef LITE_DATA
        if (buf->no_sub) {
#endif
            if (*str == 's' || *str == 'v') *str = '-';
#ifndef LITE_DATA
        }
        else {
            if (*str == 's') {
                if (last_verb < 0 || last_con > 0 || esyl < 4) {
                    *str = '-';
                    continue;
                }
                dsyl=0;
                last_con = str-buf->inptr;
                continue;
            }
            if (*str == 'v') {
                if (last_verb >= 0 && last_con >= 0) {
                    buf->inptr[last_verb] = '-';
                    last_con = -1; // pozostawiamy informację
                    esyl = dsyl;
                }
                last_verb = str-buf->inptr;
                
            }
        }
#endif
    }
#ifndef LITE_DATA
    if (last_verb > 0) buf->inptr[last_verb]='-';
    if (last_con > 0) buf->inptr[last_con]='-';
#endif            
            
    
    str = buf->inptr;
    
    int pk=0;
    for (word_count=0; word_count < 32;) {
        str=getWordInfo(str, &winfo[word_count], pk);
        if (!str) break;
        pk = winfo[word_count].flags & WINFO_KEEP;
        word_count++;
        if (winfo[word_count-1].flags & WINFO_LAST) break;
    }
    if (word_count == 32 && !(winfo[31].flags & WINFO_LAST)) return -1;
    for (i=0; i < word_count-1;i++) {
        if (!(winfo[i].flags & WINFO_KEEP)) continue;
        if (winfo[i+1].flags & WINFO_STRESSPREV) {
            if (winfo[i].syllables >= winfo[i+1].prim_stress) {
                winfo[i].prim_stress = winfo[i].syllables + 1 - winfo[i+1].prim_stress;
            }
            else {
                winfo[i].prim_stress = 1;
            }
            winfo[i+1].prim_stress = 0;
            winfo[i].sec_stress = 0;
        }
        else {
            winfo[i].sec_stress = (winfo[i].syllables > 1) ? winfo[i].prim_stress : 0;
            winfo[i].prim_stress = 0;
        }
    }
    int cw, csyl;
    for (cw = 0;cw < word_count; cw++) {
        while (*buf->inptr && microlena_isspace(*buf->inptr)) buf->inptr++;
        if (*buf->inptr == '[') {
            buf->inptr = strchr(buf->inptr,']') + 1;
        }
        blank(buf);
        if (winfo[cw].flags & WINFO_EXSTRES) {
            //printf("External stress\n");
            while (*buf->inptr && !microlena_isspace(*buf->inptr)) {
                pushout(buf,*buf->inptr++);
            }
            continue;
        }
        if (winfo[cw].flags & (WINFO_SEP | WINFO_BRK)) {
            pushout(buf,'[');
            if (winfo[cw].flags & WINFO_BRK) pushout(buf,'b');
            if (winfo[cw].flags & WINFO_SEP) pushout(buf,'s');
            pushout(buf,']');
        }
        for (csyl = 0;;) {
            if (!*buf->inptr || microlena_isspace(*buf->inptr)) break;
            if (strchr(EPN_VOWEL, *buf->inptr)) {
                csyl++;
                if (csyl == winfo[cw].prim_stress) pushstr(buf,"~!",0);
                else if (csyl == winfo[cw].sec_stress) pushstr(buf,"~,",0);
            }
            pushout(buf,*buf->inptr++);
        }
    }
    pushout(buf,0);
    return 1;
}
