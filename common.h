#ifndef MICROLENA_COMMON_H
#define MICROLENA_COMMON_H
#include <stdint.h>



#define pushstr(buf, a, b) do {if (!(buf)->_pushstr(buf, (a), (b))) return -1;} while (0)
#define pushout(buf, a) do {if (!(buf)->_pushout(buf, (a))) return -1;} while (0)
#define blank(buf) do {if((buf)->outptr != (buf)->buffer && \
     (buf)->outptr[-1] != ' ' && \
     !(buf)->_pushout((buf),' ')) \
     return -1;} while (0)
// interfejs

#ifdef __cplusplus
extern "C" {
#endif

struct syl_env {
    unsigned int stress:4;
    unsigned int npitch:4;
    unsigned int marked:4;
    unsigned int pitch1:10;
    unsigned int pitch2:10;
    float pitches[5];
    uint16_t offsets[5];
    uint8_t env;
};

struct mbrophon {
    uint8_t i1;
    uint8_t i2;
    uint8_t o1;
    uint8_t o2;                 // najstarszy bit - dodatkowy apostrof
    unsigned int length:10;
    unsigned int nrules:6;
    unsigned int begrule:10;
    unsigned int vowel:1;
};

struct mbrolizer {
    struct syl_env *syllables;
    int16_t syl_count;
    int16_t geminator;
    int16_t this_syl;
    int16_t last_vowel_len;


    unsigned int next_dup:1;
    unsigned int finished:1;
    unsigned int same_vowel:2;
    unsigned int ptyp:4;

    int8_t last_type;
    
    uint8_t ring_buffer_len;
    uint8_t ring_buffer_pos;
    const char *str;
    
    const char *line;
    char ring_buffer[128];
    const struct mbrophon *prev_pho;
};

struct microlena_Buffer {
    int bufsize;
    int16_t margin;
    uint8_t currentPhrase;
    unsigned int readmode:1;
    unsigned int ft:1;
    unsigned int no_sub:1;
    unsigned int show_phonemes:1;
    unsigned int simple_decipoint:1;
    unsigned int alt_colon:1;

    struct mbrolizer mbrol;
    char *inbuf;
    char *inbufptr;
    char *inptr;
    char *outptr;
    int (*_pushstr)(struct microlena_Buffer *, const char *, int);
    int (*_pushout)(struct microlena_Buffer *, char);
    char decipoint[16];
    char buffer[4];
};


extern  void microlena_simpleDeciPoint(struct microlena_Buffer *buf, int dp);
extern int microlena_setDeciPoint(struct microlena_Buffer *buf, char *c);

extern int microlena_utf2iso(const char *instr,char *outstr);
extern void microlena_ShiftBuffer(struct microlena_Buffer *buf);
extern struct microlena_Buffer *microlena_InitBuffer(const char *text, int convert);
extern void microlena_StartTextBuffer(struct microlena_Buffer *buf);


// przydatne

extern int microlena_isalnum(int c);
extern int microlena_isupper(int c);
extern int microlena_tolower(int c);
extern int microlena_isvowel(int c);
extern int microlena_isalpha(int c);

#define microlena_isdigit(a) (a && isdigit((int)(a) & 255))
#define microlena_isspace(a) (a && isspace((int)(a) & 255))

extern void microlena_setUserDict(const char * const *units, const char *const *dict);


extern int microlena_Phraser(struct microlena_Buffer *buf);
extern int microlena_SpeakNumber(struct microlena_Buffer *buf, int n, int typ);
extern int microlena_SpeakNumberS(struct microlena_Buffer *buf, int n,const char **fmt);
extern int microlena_integer(struct microlena_Buffer *buf, char **inps);
extern int microlena_int(struct microlena_Buffer *buf, int n);
extern int microlena_Spell(struct microlena_Buffer *buf, int count);
extern int microlena_SpellChar(struct microlena_Buffer *buf, char c, int *bg);


// mbrola
extern int microlena_getMbrolaPhone(struct microlena_Buffer *buf, char *outbuf);
extern int microlena_Intonator(struct microlena_Buffer *buf);

// main

extern int microlena_getPhonemes(struct microlena_Buffer *buf, char *outbuf);


enum {
    PHRA_COMMA = 1,
    PHRA_DOT,
    PHRA_COLON,
    PHRA_EXCLA,
    PHRA_QUE,
    PHRA_ELLIP
};

extern int microlena_eoph(const char *in, const char **out);

// data
extern int microlena_match_udict(struct microlena_Buffer *buf);
extern int microlena_match_recognizer(struct microlena_Buffer *buf);

// prestresser

extern int microlena_WordClass(char *word, uint8_t *stress, uint8_t *stressp);
extern int microlena_PreStress(struct microlena_Buffer *buf);
extern void microlena_CorrectPrestresser(struct microlena_Buffer *buf);
extern char *microlena_LocString(unsigned int pos);
extern int microlena_Translate(struct microlena_Buffer *buf);
extern int microlena_Poststresser(struct microlena_Buffer *buf);

// c++
#ifdef __cplusplus
}
#endif

 #define MILD_VERB  0x0001
 #define MILD_PPAS  0x0002
 #define MILD_OPER  0x0004
 #define MILD_SEP   0x0008
 #define MILD_U     0x0010
 #define MILD_P     0x0020
 #define MILD_R     0x0040
 #define MILD_K     0x0080
 #define MILD_COND  0x0100
 #define MILD_STRE  0x0200
 #define MILD_NOOP  0x0400
 #define MILD_BREAK 0x0800
// marker - nie używany
#define MILD_FOUND 0x8000
#define MILD_NOWORD 0x4000
#define MILD_NOUNSTRESS 0x2000

//

#endif
