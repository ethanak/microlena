#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"


#define FMOD_SEPARATOR 1
#define FMOD_GEMINATOR 2
#define FMOD_BIG_N 3
#define FMOD_SML_N 4
#define FMOD_PALATIZER 5
#define FMOD_CONNECTOR 6
#define FMOD_CRG_S  7
#define FMOD_CRG_H  8
#define FMOD_CRG_SI  9
#define FMOD_CRG_SZ  10
#define FMOD_CRG_ZH  11
#define FMOD_CRG_Z  12
#define FMOD_CRG_F  13
#define FMOD_CRG_Y  14

//punctone[8] = {0,1,3,2,5,4,0,0};
//punctone_ew[8]={0,1,0,4,2,3,5,0};


struct voice_param {
    double base_pitch;
    double range;
};

struct env_data {
    struct env_data *next;
    const char *name;
    int count;
    int offset[8];
    double value[8];
};

struct tone_table {

//   struct env_data *pitch_env0;     /* pitch envelope, tonic syllable at end */
//   struct env_data *pitch_env1;     /*     followed by unstressed */

    uint8_t pitch_env0;         /* pitch envelope, tonic syllable at end */
    uint8_t pitch_env1;         /*     followed by unstressed */

    unsigned char tonic_max0;
    unsigned char tonic_min0;

    unsigned char tonic_max1;
    unsigned char tonic_min1;

    unsigned char pre_start;
    unsigned char pre_end;

    unsigned char body_start;
    unsigned char body_end;

    unsigned char body_max_steps;
    unsigned char body_lower_u;

    unsigned char tail_start;
    unsigned char tail_end;
};

struct intonator {
    struct voice_param vp;
    //struct env_data *rise,*fall,*env,*singles[6];
    uint8_t rise, fall, env, singles[6];
    struct tone_table tone_table[6];
    int pitch_range2;
    int pitch_base2;

};
/*
struct mbrophon {
    uint8_t i1;
    uint8_t i2;
    uint8_t o1;
    uint8_t o2; // najstarszy bit - dodatkowy apostrof
    unsigned int length:10;
    unsigned int vowel:1;
    unsigned int nrules:4;
    unsigned int begrule:10;
};
*/

struct mbr_modifier {
    uint8_t prev;
    uint8_t next;
    uint8_t flags;
    uint8_t add;
};
#include "pl_mbrola.h"

static struct mbrophon PHO_PAUSE = {
    '_', 0, '_', 0, 1, 0, 0, 0
};

/*
#define STENV_RISE &env_static[0]
#define STENV_FALL &env_static[1]
#define STENV_FRISE &env_static[2]
#define STENV_FRIS2 &env_static[3]
#define STENV_DROP &env_static[4]
#define STENV_QUESTION &env_static[5]
*/

// zero do dawniej NULL
#define STENV_RISE 1
#define STENV_FALL 2
#define STENV_FRISE 3
#define STENV_FRIS2 4
#define STENV_DROP 5
#define STENV_QUESTION 6

static struct env_data env_static[] = {
    {&env_static[1], "rise", 2, {0, 100}, {0.0, 1.0}},
    {&env_static[2], "fall", 2, {0, 100}, {1.0, 0}},
    {&env_static[3], "frise", 5, {0, 17, 35, 68, 100},
     {1.0, 0.24, 0.0, 0.03, 0.09}},
    {&env_static[4], "fris2", 5, {0, 36, 73, 87, 100},
     {1.0, 0.24, 0.0, 0.03, 0.0125}},
    {&env_static[5], "drop", 5, {0, 24, 50, 75, 100},
     {1.0, 0.78, 0.566, 0.19, 0.0}},
    {NULL, "question", 3, {0, 50, 100}, {0.0, 2.0, 0.0}}
};

static struct intonator intonator_static = {
    {1.0, 1.0},                 // voice param
    STENV_RISE,
    STENV_FALL,
    1,                          //&env_static[0],
    {0, 0, 0,
     STENV_QUESTION,
     0, 0},                     // singles
    {
     {STENV_FALL,
      STENV_FALL,
      30, 5, 30, 7,             // statement
      20, 25, 34, 22, 3, 3, 12, 8},

     {STENV_FRISE,
      STENV_FRIS2,
      38, 10, 36, 10,           // comma
      20, 25, 34, 20, 3, 3, 15, 25},

     {STENV_FRISE,
      STENV_DROP,
      38, 1, 42, 25,            // exclamation
      20, 25, 34, 22, 3, 3, 12, 8},

     {STENV_FRISE,
      STENV_FRIS2,
      38, 10, 36, 10,           // question
      20, 25, 34, 20, 13, 13, 15, 65},

     {STENV_FRISE,
      STENV_FALL,
      38, 1, 42, 25,            // colon
      20, 25, 34, 22, 3, 3, 12, 10},

     {STENV_FALL,
      STENV_FALL,
      30, 5, 30, 7,             // ellipsis
      20, 25, 22, 18, 3, 3, 13, 22},

     },                         // tonetables
    266, 130
};

struct pitcher_data {
    struct syl_env *syllables;
    int count;
    int clause_tone;
    int tone_type;
    int number_pre;
    int number_body;
    int number_tail;
    int tone_posn;
    int annotation;
    int last_primary;
};

#define STRESS_SECONDARY  3
#define STRESS_PRIMARY    4
#define STRESS_PRIMARY_MARKED 6
#define STRESS_BODY_RESET 7
#define STRESS_FIRST_TONE 8     /* first of the tone types */

/* intonator Duddingtona */
//#if 0
static void count_pitch_vowels(struct pitcher_data *pd)
{
    int i, stres;
    pd->tone_posn = 0;
    for (i = 0; i < pd->count; i++) {
        stres = pd->syllables[i].stress & 0x3f;
        if (stres >= STRESS_PRIMARY)
            break;
    }
    pd->number_pre = i;
    for (i = pd->count - 1; i >= 0; i--) {
        stres = pd->syllables[i].stress & 0x3f;
        if (stres >= STRESS_PRIMARY)
            break;
    }
    if (i<0) i=0;
    pd->tone_posn = i;
    pd->syllables[pd->tone_posn].stress = STRESS_FIRST_TONE;
    pd->number_tail = pd->count - pd->tone_posn - 1;

}

static int count_increments(struct pitcher_data *pd, int ix, int end_ix,
                            int min_stress)
/*************************************************************/
/* Count number of primary stresses up to tonic syllable or body_reset */
{
    int count = 0;
    int stress;

    while (ix < end_ix) {
        stress = pd->syllables[ix++].stress & 0x3f;
        if (stress >= STRESS_BODY_RESET)
            break;
        else if (stress >= min_stress)
            count++;
    }
    return (count);
}                               /* end of count_increments */

static void set_pitch(struct pitcher_data *pd, int ix, int base, int drop)
/***********************************************/
// Set the pitch of a vowel in vowel_tab.  Base & drop are Hz * 256
{
    int pitch1, pitch2;

    if (base < 0)
        base = 0;

    pitch2 =
        ((base * intonator_static.pitch_range2) >> 15) +
        intonator_static.pitch_base2;

    if (drop < 0) {
        pd->syllables[ix].env = intonator_static.rise;
        drop = -drop;
    } else
        pd->syllables[ix].env = intonator_static.fall;

    pitch1 = pitch2 + ((drop * intonator_static.pitch_range2) >> 15);

    if (pitch1 > 511)
        pitch1 = 511;
    if (pitch2 > 511)
        pitch2 = 511;

    if (pitch1 > pitch2) {
        int x = pitch1;
        pitch1 = pitch2;
        pitch2 = x;
    }

    pd->syllables[ix].pitch1 = pitch1;
    pd->syllables[ix].pitch2 = pitch2;
}                               /* end of set_pitch */

static int calc_pitch_segment2(struct pitcher_data *pd, int ix, int end_ix,
                               int start_p, int end_p)
/****************************************************************************************/
/* Linear pitch rise/fall, change pitch at min_stress or stronger
	Used for pre-head and tail */
{
    int stress;
    int pitch;
    int increment;
    int n_increments;
    int drop;
    static int min_drop[] =
        { 0x300, 0x300, 0x300, 0x300, 0x300, 0x500, 0xc00, 0xc00 };

    if (ix >= end_ix)
        return (ix);

    n_increments = count_increments(pd, ix, end_ix, 0);
    increment = (end_p - start_p) << 8;

    if (n_increments > 1) {
        increment = increment / n_increments;
    }

    pitch = start_p << 8;
    while (ix < end_ix) {
        stress = pd->syllables[ix].stress & 0x3f;

        if (increment > 0) {
            set_pitch(pd, ix, pitch, -increment);
            pitch += increment;
        } else {
            drop = -increment;
            if (drop < min_drop[stress])
                drop = min_drop[stress];

            pitch += increment;
            set_pitch(pd, ix, pitch, drop);
        }

        ix++;
    }
    return (ix);
}                               /* end of calc_pitch_segment2 */

static int calc_pitch_segment(struct pitcher_data *pd,
                              int ix,
                              int end_ix, struct tone_table *t, int min_stress)
/******************************************************************************/
/* Calculate pitches until next RESET or tonic syllable, or end.
	Increment pitch if stress is >= min_stress.
	Used for tonic segment */
{
    int stress;
    int pitch = 0;
    int increment = 0;
    int n_primary = 0;
    int initial;
    int overflow = 0;

    static char overflow_tab[5] = { 0, 5, 3, 1, 0 };
    static int drops[] =
        { 0x400, 0x400, 0x700, 0x700, 0x700, 0xa00, 0x0e00, 0x0e00 };

    initial = 1;
    while (ix < end_ix) {
        stress = pd->syllables[ix].stress & 0x3f;

        if (stress == STRESS_BODY_RESET)
            initial = 1;

        if (initial || (stress >= min_stress)) {
            if (initial) {
                initial = 0;
                overflow = 0;
                n_primary = count_increments(pd, ix, end_ix, min_stress);

                if (n_primary > t->body_max_steps)
                    n_primary = t->body_max_steps;

                if (n_primary > 1) {
                    increment = (t->body_end - t->body_start) << 8;
                    increment = increment / (n_primary - 1);
                } else
                    increment = 0;

                pitch = t->body_start << 8;
            } else {
                if (n_primary > 0)
                    pitch += increment;
                else {
                    pitch =
                        (t->body_end << 8) -
                        (increment * overflow_tab[overflow++]) / 4;
                    if (overflow > 4)
                        overflow = 0;
                }
            }
            n_primary--;
        }

        if (stress >= STRESS_PRIMARY) {
            pd->syllables[ix].stress = STRESS_PRIMARY_MARKED;
            set_pitch(pd, ix, pitch, drops[stress]);
        } else if (stress >= STRESS_SECONDARY) {
            /* use secondary stress for unmarked word stress, if no annotation */
            set_pitch(pd, ix, pitch, drops[stress]);
        } else {
            /* unstressed, drop pitch if preceded by PRIMARY */
            if ((pd->syllables[ix - 1].stress & 0x3f) >= STRESS_SECONDARY)
                set_pitch(pd, ix, pitch - (t->body_lower_u << 8),
                          drops[stress]);
            else
                set_pitch(pd, ix, pitch, drops[stress]);
        }

        ix++;
    }
    return (ix);
}                               /* end of calc_pitch_segment */

static struct env_data *calc_pitches(struct pitcher_data *pd)
/********************************************************************/
/* Calculate pitch values for the vowels in this tone group */
{
    int ix;
    struct tone_table *t;
    int drop;
    struct env_data *tone_pitch_env;

    t = &intonator_static.tone_table[pd->tone_type];
    ix = 0;

    /* vowels before the first primary stress */
        /******************************************/

    if (pd->number_pre > 0) {
        ix = calc_pitch_segment2(pd, ix, ix + pd->number_pre, t->pre_start,
                                 t->pre_end);
    }

    /* body of tonic segment */
                                /*************************/
    ix = calc_pitch_segment(pd, ix, pd->tone_posn, t, STRESS_PRIMARY);

    if (pd->number_tail == 0) {
        tone_pitch_env = &env_static[t->pitch_env0 - 1];
        drop = t->tonic_max0 - t->tonic_min0;
        set_pitch(pd, ix++, t->tonic_min0 << 8, drop << 8);
    } else {
        //printf("PENV %d, \n",t->pitch_env1-1);
        tone_pitch_env = &env_static[t->pitch_env1 - 1];
        drop = t->tonic_max1 - t->tonic_min1;
        set_pitch(pd, ix++, t->tonic_min1 << 8, drop << 8);
    }

    /* tail, after the tonic syllable */
        /**********************************/

    calc_pitch_segment2(pd, ix, pd->count, t->tail_start, t->tail_end);

    return tone_pitch_env;
}                               /* end of calc_pitches */

static void compute_pitches(struct syl_env *syllables, int count,
                            int clause_tone)
{
    struct pitcher_data pd;
    int tonic_ix = -1;
    int tonic_iy = -1;
    struct env_data *tonic_env;
    int ix, max_stress = 0;
//      static unsigned char punctone[8] = {0,1,0,4,2,3,5,0};;
    //static uint8_t punctone[8] = { 0, 1, 0, 4, 2, 3, 5, 0 };
    static uint8_t punctone[8] = { 0, 1, 0, 4, 2, 3, 5, 0 };
    pd.syllables = syllables;
    pd.count = count;
    pd.clause_tone = clause_tone;
    pd.tone_type = punctone[clause_tone];
    for (ix = 0; ix < count; ix++)
        if (syllables[ix].stress >= max_stress) {
            max_stress = syllables[ix].stress;
            tonic_iy = tonic_ix;
            tonic_ix = ix;
        }
    if (count > 1 && tonic_ix == count - 1) {
        if (pd.tone_type == 2) {
            if (count == 2) {
                syllables[0].stress = STRESS_PRIMARY;
                syllables[1].stress = 0;
                tonic_ix = 0;
            } else {
                if (tonic_iy < count - 3)
                    tonic_iy = count - 2;
                syllables[count - 1].stress = STRESS_SECONDARY;
                syllables[tonic_iy].stress = STRESS_PRIMARY;
                tonic_ix = tonic_iy;
            }
        }
    } else if (count >= 4 && tonic_ix == count - 4) {
        syllables[tonic_ix].stress = STRESS_SECONDARY;
        tonic_ix += 2;
        syllables[tonic_ix].stress = STRESS_PRIMARY;
    }
    count_pitch_vowels(&pd);
    tonic_env = calc_pitches(&pd);
    /* nie bedzie singles */
    if (count == 1) {
        int n = intonator_static.singles[pd.tone_type];

        struct env_data *ev = (n) ? &env_static[n - 1] : NULL;
        if (ev) {
            tonic_env = ev;
            tonic_ix = 0;
        }
    }
    if (tonic_ix >= 0) {
        syllables[tonic_ix].env =
            tonic_env ? ((tonic_env - &env_static[0]) + 1) : 0;
    }
    /* tu juz mamy pitch1 i pitch2 i env */
    for (ix = 0; ix < count; ix++) {
        struct env_data *ev = &env_static[syllables[ix].env - 1];
        int i;
        double base;
        double range;
        syllables[ix].npitch = ev->count < 5 ? ev->count : 5;
        base = intonator_static.vp.base_pitch * syllables[ix].pitch1;
        range =
            intonator_static.vp.base_pitch * intonator_static.vp.range *
            (syllables[ix].pitch2 - syllables[ix].pitch1);
        if ((syllables[ix].marked & 1) && ix != tonic_ix) {
            range *= 2;
            base *= 1.05;
        }
        for (i = 0; i < ev->count; i++) {
            syllables[ix].offsets[i] = ev->offset[i];
            syllables[ix].pitches[i] = base + range * ev->value[i];
        }
    }
}

//#endif
/* koniec intonatora Duddingtona */



static const struct mbrophon *get_phonem(const char *str, const char **outstr,
                                         uint8_t * stress, uint8_t * sep)
{
    int i, srs = 0, se = 0;
    if (!str) return NULL;
    while (*str) {
        if (*str == '~') {
            str++;
            if (*str == ',')
                srs = 1;
            else if (*str == '!')
                srs = 2;
            else if (*str == '?')
                srs = 3;
            if (*str)
                str++;
            continue;
        }
        if (*str == ' ') {
            str++;
            continue;
        }
        if (*str == '[') {
            while (*str && *str != ']') {
                if (*str == 's')
                    se = 1;
                if (*str == 'b')
                    se = 2;
                str++;
            }
            
            if (!*str)
                return NULL;
            str++;
        //    printf("Vave ] %s\n",str);
            continue;
        }
        break;
    }
    if (!*str)
        return NULL;
    for (i = 0; i < mbrophon_count; i++) {
        if ((*str & 255) != mbrophon[i].i1)
            continue;
        if (mbrophon[i].i2 && (str[1] & 255) != mbrophon[i].i2)
            continue;
        if (outstr)
            *outstr = str + 1 + (mbrophon[i].i2 ? 1 : 0);
        if (stress)
            *stress = srs;
        if (sep && se)
            *sep = se;
        return &mbrophon[i];
    }
    //printf("Unknown at <%s>\n", str);
    return NULL;
}



#define mkflag 6

void align4(char **adr)
{
    
    int delta = ((long int)(*adr)) & 3;
    if (delta) (*adr) += 4-delta;
}


static void *allocSylEnv(struct microlena_Buffer *buf, int count)
{
    char *adr = buf->buffer;
    align4(&adr);
    int icnt = (buf->inptr - adr) / sizeof(struct syl_env);
    if (icnt < count) return NULL;
    return (void *)adr;
}


int microlena_Intonator(struct microlena_Buffer *buf)
{
    const char *str;
    const struct mbrophon *mbp;
    static uint8_t smodes[] =
        { 0, STRESS_SECONDARY, STRESS_PRIMARY, STRESS_PRIMARY_MARKED };

    memset(&buf->mbrol, 0, sizeof(struct mbrolizer));
    buf->mbrol.geminator = -1;

    if (microlena_isdigit(*buf->inptr)) {
        buf->inptr++;
        if (*buf->inptr == ':') buf->inptr++;
    }
    str = buf->inptr;
    struct syl_env *syllables;
    int syl_count, i, no_phones;
    for (syl_count = no_phones = 0;;) {
        if (!(mbp = get_phonem(str, &str, NULL, NULL)))
            break;
        no_phones++;
        if (mbp->vowel)
            syl_count++;
    }

    if (!(syllables = allocSylEnv(buf, syl_count+4))) {
        return -1;
    }
    
    //syllables = calloc(sizeof(*syllables), syl_count + 4);

    //("ACNT %p\n",buf->inbufptr);
    

    str = buf->inptr;
    uint8_t stres, sep;
    for (i = 0, sep=0;;) {
        if (!(mbp = get_phonem(str, &str, &stres, &sep)))
            break;
        if (!mbp->vowel)
            continue;
        if (stres == 3) {
            stres = 2;
            syllables[i].marked |= 1;
        }
        if (sep) {
            syllables[i].marked |= (sep == 1) ? 2 : 4;
            sep=0;
        }
        syllables[i++].stress = smodes[stres];
    }
    
    if (syl_count == 1)
        buf->mbrol.geminator = 0;
    else if (buf->mbrol.geminator < 0 && syllables[syl_count - 1].stress) {
        syllables[syl_count - 2].stress = 0;
        buf->mbrol.geminator = syl_count - 1;
    }
    if (buf->mbrol.geminator >= 0) {
        syllables[buf->mbrol.geminator].stress = smodes[2];
        syllables[syl_count++].stress = 0;
    } else {
        int i;
        /* ostatni akcent jest zawsze podstawowy */
        for (i = syl_count - 1; i >= 0; i--)
            if (syllables[i].stress) {
                syllables[i].stress = smodes[2];
                break;
            }
    }

//    printf("BCNT %p\n",buf->inbufptr);
    /* tu mamy wszystkie akcenty? Teraz separatory */
    int pos = 0;
    while (pos < syl_count) {
        int lst;
        for (i = pos, lst = 0; i < syl_count; i++) {
            if (syllables[i].stress)
                lst = i;
            if (syllables[i].marked & mkflag) {
                int mdif = (syllables[i].marked & 4) ? 1 : 4;
                if (i < pos + mdif || i > syl_count - mdif) {
                    if (!pos || i != pos)
                        syllables[i].marked &= ~6;
                    continue;
                }
                break;
            }
        }
//punctone[8] =  {0,1,3,2,5,4,0,0};
//punctone_ew[8]={0,1,0,4,2,3,5,0};
//    printf("XCNT %p\n",buf->inbufptr);
    
        if (i < syl_count) {
            int nmode = 1, j;
            for (j = i; j < syl_count; j++) {
                if (j < i + 1 || j > syl_count - 1)
                    continue;
                if (syllables[j].marked & 4)
                    break;
                if (j < i + 4 || j > syl_count - 4)
                    continue;
                if (syllables[j].marked & 2)
                    break;
            }
            //if (j == syls && i>= syls-5 && ((pmode & 7)==1 || (pmode & 7) ==2)) nmode=5;

            //fprintf(stderr,"%d %d %d %d %d\n",pos,i,j,syls,nmode);
            //printf("LST=%d/%d i=%p\n",lst,syl_count,buf->inbufptr);
            if (lst)
                syllables[lst].stress = smodes[2];
            //printf("CPI %d %d %d\n", pos, i - pos, nmode);
            compute_pitches(syllables+pos,i-pos,nmode);
            //printf("CPIOK\n");
            pos = i;
            //printf("YCNT %p\n",buf->inbufptr);
        } else
            break;
    }
//printf("CCNT %p\n",buf->inbufptr);
    //fprintf(stderr,"%x %x\n",pmode,vnmode);
    if (pos < syl_count) {
        //printf("CPI %d %d %d\n", pos, syl_count - pos, ptyp);
        compute_pitches(syllables + pos, syl_count - pos,
            (buf->alt_colon && buf->currentPhrase == PHRA_COLON) ? PHRA_COMMA: buf->currentPhrase);
    }
    buf->mbrol.syllables = syllables;
    buf->mbrol.str = buf->inptr;
    buf->mbrol.line = buf->inptr;
    buf->mbrol.syl_count = syl_count;
    buf->mbrol.ptyp = buf->currentPhrase;
    return 1;
}

#define FLAG_ADD 0x80
#define FLAG_SUB 0x40
#define FLAG_SEPARATOR 0x20
#define FLAG_CMASK 0x0f

#define FMOD_NONE 0
#define FMOD_PSEUDOWOVEL 1
#define FMOD_GEMINATOR 2
#define FMOD_BIG_N 3
#define FMOD_SML_N 4
#define FMOD_PALATIZER 5
#define FMOD_CONNECTOR 6
#define FMOD_CRG_S  7
#define FMOD_CRG_H  8
#define FMOD_CRG_SI  9
#define FMOD_CRG_SZ  10
#define FMOD_CRG_ZH  11
#define FMOD_CRG_Z  12
#define FMOD_CRG_F  13
#define FMOD_CRG_Y  14
#define FMOD_NOMOD 0
#define FMOD_CRG_AH 15

#define PAUSE_CONJ 20
#define PAUSE_NORMAL 240

#define PHO_IDX(pho) ((pho == &PHO_PAUSE) ? 0 : (pho - mbrophon))
#define PHOT_START 0x80
#define PHOT_END 0x81
#define PHOT_VOWEL 0x82
#define PHOT_ANY 0x83
#define PHOT_NONE 0xff

#define length_palatizer 5
#define length_extrai 25
#define length_extraj 1
#define length_extraf 1
#define length_extras 4
#define length_extrah 5
#define length_geminator 5
#define length_separator 2
#define length_minlast 140
#define length_stress 20

static void send_buffer(struct microlena_Buffer *buf, const char *c)
{
    while (*c) {
        int n = (buf->mbrol.ring_buffer_pos + buf->mbrol.ring_buffer_len) & 127;
        if (*c == '\n') {
            buf->mbrol.ring_buffer[n] = 0;
            buf->mbrol.ring_buffer_len ++;
            break;
        }
        else {
            buf->mbrol.ring_buffer[n] = *c++;
            buf->mbrol.ring_buffer_len ++;
        }
    }
}
static void finalize(struct microlena_Buffer *buf, const char *phone, int len)
{
    char buffer[32];
    if (phone) sprintf(buffer, "%s %d\n",phone,len);
    else buffer[0] = '\n';
    send_buffer(buf, buffer);
}

static void mbrflush(struct microlena_Buffer *buf)
{
    send_buffer(buf,"#\n");
}

static void psampa(struct microlena_Buffer *buf, const struct mbrophon *pho,int len, int final)
{
    char buffer[32], *c=buffer;
    *c++=pho->o1;
    if (pho->o2)
        *c++ =  pho->o2 & 127;
    if (pho->o2 & 128)
        *c++='\'';
    
    c+=sprintf(c," %d", len);
    if (final) *c++='\n';
    send_buffer(buf, buffer);
}

static void phb42(struct microlena_Buffer *buf,int n, double d)
{
    int t = 100 * d;
    const char *c = "";
    if (t < 0) {
        t = -t;
        c = "-";
    }
    char buffer[32];
    sprintf(buffer," %d %s%d.%02d", n, c, t / 100, t % 100);
    send_buffer(buf, buffer);
}

static int extra_phonem(struct microlena_Buffer *buf, const char *ph,const struct mbrophon *next_pho,const struct mbrophon *prev_pho)
{
	int phlen,xflags;int i;
	const struct mbrophon *pho;

    for (i = 0; i < mbrophon_count; i++) {
        if ((*ph & 255) != mbrophon[i].i1)
            continue;
        if (mbrophon[i].i2 && (ph[1] & 255) != mbrophon[i].i2)
            continue;
        break;
    }
    if (i>= mbrophon_count) {
        printf("Mbrola: niezdefiniowany fonem '%s'\n",ph);
		return 0;
    }
    pho = &mbrophon[i];
    if (pho->vowel) {
		printf("Mbrola: ekstra fonem '%s' to samogloska\n",ph);
		return 0;
	}
	phlen=pho->length;
	xflags=0;


    const struct mbr_modifier *rule = NULL;
    for (i = 0; i < pho->nrules; i++) {
        rule = &mbr_modifier[pho->begrule + i];
        if (rule->prev != 0xff) {
            if (rule->prev == PHOT_START) continue;
            if (rule->prev == PHOT_VOWEL) {
                if (!prev_pho->vowel) continue;
            } else {
                if (PHO_IDX(prev_pho) != rule->prev) continue;
            }
        }
        if (rule->next == 0xff || rule->next == PHOT_ANY)
            break;
        if (rule->next == PHOT_END) {
            if (next_pho) break;
            continue;
        }
        if (!next_pho) continue;
        if (rule->next == PHOT_VOWEL) {
            if (next_pho->vowel) break;
            continue;
        }
        if (PHO_IDX(prev_pho) == rule->prev) break;
    }
    if (i < pho->nrules) {
		xflags=rule->flags & FLAG_CMASK;
		if(rule->add) {
			if (rule->flags & FLAG_ADD) phlen+=rule->add;
			else if (rule->flags & FLAG_SUB) phlen-=rule->add;
			else phlen=rule->add;
		}
	}
	if (!next_pho && phlen<length_minlast) phlen=length_minlast;
    psampa(buf, pho, phlen, 1);
    return phlen;
}


/* dupa */

int microlena_sigmbro(struct microlena_Buffer *buf)
{
    const struct mbrophon *pho, *next_pho;
    uint8_t stres, sep, isep;
    const char *nstr;
    int phlen, flags;
    int i, j;

    sep=0;
    if (buf->mbrol.finished) return 0;
    pho = get_phonem(buf->mbrol.str, &buf->mbrol.str, &stres, &sep);
    if (!pho) {
        //if (buf->mbrol.last_type) finalize(buf,"x",1);
        finalize(buf,"_",(buf->currentPhrase == PHRA_COMMA) ? 20:
        (buf->currentPhrase == PHRA_COLON) ? 30:
         120);
        mbrflush(buf);
        buf->mbrol.finished=1;
        return 1;
    }
    buf->mbrol.last_type = pho->vowel;
    /* pauza mi\352dzy wyrazami? Wypisanie */
    if (sep && (buf->mbrol.syllables[buf->mbrol.this_syl].marked & mkflag)) {
        if (buf->mbrol.this_syl)
            finalize (buf, "_", 20);
            //printf("_ 20\n");
        sep = 0;
    }
    isep = 0;
    next_pho = get_phonem(buf->mbrol.str, &nstr, NULL, &isep);
    if (next_pho && isep) {
        int n = buf->mbrol.this_syl;
        if (pho->vowel)
            n++;
        if (buf->mbrol.syllables[n].marked & mkflag) {
            next_pho = &PHO_PAUSE;
        }
    }
    phlen = pho->length;
//    printf("; Len %d for %c\n",phlen, pho->o1);
        
        
    flags = 0;
    const struct mbr_modifier *rule = NULL;
    for (i = 0; i < pho->nrules; i++) {
        rule = &mbr_modifier[pho->begrule + i];
        if (rule->prev != 0xff) {
            if (rule->prev == PHOT_START) {
                if (buf->mbrol.prev_pho)
                    continue;
            } else {
                if (!buf->mbrol.prev_pho)
                    continue;
                if (rule->prev == PHOT_VOWEL) {
                    if (!buf->mbrol.prev_pho->vowel)
                        continue;
                } else {
                    if (PHO_IDX(buf->mbrol.prev_pho) != rule->prev)
                        continue;
                }
            }
        }                   // pred comparoson
        if (rule->next == 0xff || rule->next == PHOT_ANY)
            break;

        if (rule->next == PHOT_END) {
            if (!next_pho)
                break;
            if (!pho->vowel)
                continue;
            if (!next_pho->vowel)
                continue;
            if (buf->mbrol.same_vowel)
                continue;
            if (buf->mbrol.geminator == buf->mbrol.this_syl)
                continue;
            if (buf->mbrol.geminator == buf->mbrol.this_syl + 1)
                continue;
            if (pho->i1 == next_pho->i1 && pho->i2 == next_pho->i2)
                continue;
            if (pho->o2 || !strchr("aeoIi", next_pho->o1))
                continue;
            if (get_phonem(nstr, NULL, NULL, NULL))
                continue;
            break;
        }
        if (!next_pho)
            continue;
        if (rule->next == 0) {
            const struct mbrophon *phnext = next_pho;
            if (pho->vowel &&
                next_pho->vowel &&
                buf->mbrol.same_vowel &&
                buf->mbrol.geminator != buf->mbrol.this_syl &&
                buf->mbrol.geminator != buf->mbrol.this_syl + 1 &&
//                                              dup_vowel!=this_syl+1 &&
                pho->i1 == next_pho->i1 &&
                pho->i2 == next_pho->i2 &&
                !pho->o2 && strchr("aeoIi", next_pho->o1)) {
                phnext = get_phonem(nstr, NULL, NULL, NULL);
                if (!phnext)
                    continue;
            }
            if (phnext->i1 != '_')
                continue;
            break;
        }
        if (rule->next == PHOT_VOWEL) {
            if (next_pho->vowel)
                break;
            continue;
        }
        if (rule->next == PHO_IDX(next_pho))
            break;
    }
    if (i < pho->nrules) {
        //if (rule->add) printf("; (%x) %d %d %d\n", rule->flags, rule->flags & FLAG_ADD, rule->flags & FLAG_SUB, rule->add);
        if (rule->add) {
            if (rule->flags & FLAG_ADD)
                phlen += rule->add;
            else if (rule->flags & FLAG_SUB)
                phlen -= rule->add;
            else
                phlen = rule->add;
        }
        flags = rule->flags & FLAG_CMASK;
    }
    /* korekta samogłoski przed pauzą */
    if (phlen < 200 && pho->vowel && (!next_pho  || next_pho->o1 == '_')) {
        phlen=200;
    }
    

    if (pho->vowel) {
        
        int j, np;
        //if (pass) fprintf(stderr,"SYL LEN %d\n",sylenv[this_syl].syl_len);
        buf->mbrol.next_dup = 0;
        if (flags == FMOD_CONNECTOR) {
            finalize(buf,"j", length_extraj);
        }
        else if (flags == FMOD_CRG_Z) {
            finalize(buf,"z", length_extras);
        }
        else if (flags == FMOD_CRG_ZH) {
            finalize(buf,"Z", length_extras);
        }
        else if (flags == FMOD_CRG_S) {
            finalize(buf,"s", length_extras);
        }
        else if (flags == FMOD_CRG_SI) {
            finalize(buf,"s'", length_extras);
        }
        else if (flags == FMOD_CRG_SZ) {
            finalize(buf,"S", length_extras);
        }
        else if (flags == FMOD_CRG_F) {
            finalize(buf,"f", length_extraf);
        }

        if (stres) {
            phlen += length_stress * stres;
        }
        if (buf->mbrol.geminator == buf->mbrol.this_syl) {
            /*
               switch(pmode & 7) {
               case 2:      phlen=phlen*1.7;break;
               case 0: phlen=phlen*1.1;break;
               default: phlen=phlen*1.4;break;
               }
               * */
            phlen = phlen * 1.4;
        }
#if 0        
         else {
            if (!buf->mbrol.same_vowel
                && buf->mbrol.geminator != buf->mbrol.this_syl + 1 && next_pho
                && next_pho->vowel && pho->i1 == next_pho->i1
                && pho->i2 == next_pho->i2 && !pho->o2
                && strchr("aeoIi", next_pho->o1))
                buf->mbrol.same_vowel = 1;
            else if (buf->mbrol.same_vowel == 1)
                buf->mbrol.same_vowel = 2;
        }
#endif
        if (!next_pho && phlen < length_minlast)
            phlen = length_minlast;
        if (buf->mbrol.same_vowel == 1)
            buf->mbrol.last_vowel_len = phlen;
        if (buf->mbrol.same_vowel == 2) {
            psampa(buf,pho, phlen + buf->mbrol.last_vowel_len,0);
        } else if (buf->mbrol.same_vowel == 0) {
            psampa(buf,pho, phlen, 0);
        }

        if (buf->mbrol.geminator != buf->mbrol.this_syl) {
            if (buf->mbrol.same_vowel == 2) {
                for (j = 0;
                     j < buf->mbrol.syllables[buf->mbrol.this_syl - 1].npitch;
                     j++) {
                    phb42(buf,(buf->mbrol.syllables[buf->mbrol.this_syl -
                                            1].offsets[j] * 4) / 10,
                          buf->mbrol.syllables[buf->mbrol.this_syl - 1].pitches[j]
                        );
                }
                for (j = 0; j < buf->mbrol.syllables[buf->mbrol.this_syl].npitch;
                     j++) {
                    phb42(buf,(buf->mbrol.syllables[buf->mbrol.this_syl].offsets[j] *
                           4 / 10) + 60,
                          buf->mbrol.syllables[buf->mbrol.this_syl].pitches[j]
                        );
                }
            } else if (!buf->mbrol.same_vowel) {
                for (j = 0; j < buf->mbrol.syllables[buf->mbrol.this_syl].npitch;
                     j++) {
                    phb42(buf,buf->mbrol.syllables[buf->mbrol.this_syl].offsets[j],
                          buf->mbrol.syllables[buf->mbrol.this_syl].pitches[j]
                        );
                }
            }
        } else {
            int lof, mx, px, np;
            np = buf->mbrol.syllables[buf->mbrol.this_syl].npitch;
            if (np > 2)
                np--;
            for (j = 0; j < np; j++) {
                phb42(buf,(buf->mbrol.syllables[buf->mbrol.this_syl].offsets[j] * 4) /
                      10, buf->mbrol.syllables[buf->mbrol.this_syl].pitches[j]
                    );
            }
            lof =
                (buf->mbrol.syllables[buf->mbrol.this_syl].offsets[np - 1] * 4) /
                10;
            buf->mbrol.this_syl++;
            np = buf->mbrol.syllables[buf->mbrol.this_syl].npitch;
            if (np > 2)
                np = 1;
            else
                np = 0;
            mx = 6;
            px = 40;
            if ((buf->mbrol.syllables[buf->mbrol.this_syl].offsets[j] * 6 / 10) +
                40 <= lof) {
                mx = 4;
                px = 60;
            }
            for (j = np; j < buf->mbrol.syllables[buf->mbrol.this_syl].npitch; j++) {
                phb42(buf,(buf->mbrol.syllables[buf->mbrol.this_syl].offsets[j] * mx /
                       10) + px,
                      buf->mbrol.syllables[buf->mbrol.this_syl].pitches[j]
                    );
            }
        }

        if (buf->mbrol.same_vowel != 1) {
            finalize(buf,NULL, 0);
            
            if (buf->mbrol.same_vowel == 2)
                buf->mbrol.same_vowel = 0;
            buf->mbrol.this_syl++;
        }
    } else if (phlen) {
        



        if (flags == FMOD_CRG_Y) {
            finalize(buf, "I", length_extrai);
        }
        if (flags == FMOD_CRG_H) {
            finalize(buf, "x", length_extrah);
        }

        if (buf->mbrol.next_dup) {
            psampa(buf,pho, length_geminator, 1);
        }
        psampa(buf,pho, phlen, 1);
    }
    buf->mbrol.next_dup = (flags == FMOD_GEMINATOR) ? 1 : 0;
    if (flags == FMOD_CRG_AH) {
        finalize(buf,"x",1);
    }
    else if (flags == FMOD_SML_N) {
        extra_phonem(buf, "n", next_pho, pho);
    } else if (flags == FMOD_BIG_N) {
        extra_phonem(buf, "N", next_pho, pho);
    }
    if (flags == FMOD_PALATIZER) {
        finalize(buf, "i", length_palatizer);
    }
    //if (flags == FMOD_CONNECTOR) {
    //    finalize(buf,"j",length_extraj);
    //}
    if (flags == FMOD_SEPARATOR) {
        if (next_pho) {
            finalize(buf, "_", length_separator);
        }
    }
    buf->mbrol.prev_pho = pho;
    return 1;

}

int microlena_getMbrolaPhone(struct microlena_Buffer *buf, char *outbuf)
{
    if (!buf->mbrol.ring_buffer_len) {
        int rc=microlena_sigmbro(buf);
        if (!rc) return 0;
    }
    char *dd=outbuf;
    while(buf->mbrol.ring_buffer_len) {
        char c = buf->mbrol.ring_buffer[buf->mbrol.ring_buffer_pos];
        buf->mbrol.ring_buffer_pos = (buf->mbrol.ring_buffer_pos + 1) & 127;
        buf->mbrol.ring_buffer_len--;
        if (!(*outbuf++ = c)) return 1;
    }
    *outbuf++=0;
    return 1;
}
