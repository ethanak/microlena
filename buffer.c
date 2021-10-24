#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "common.h"

static int _can_push(struct microlena_Buffer *buf)
{
    if (buf->readmode) {
        if (buf->outptr >= buf->buffer+buf->bufsize) return 0;
    }
    else {
        if (buf->outptr >= buf->inptr) return 0;
    }
    return 1;
}
 
static int _pushstr(struct microlena_Buffer *buf, const char *c,  int correct_blank)
{
    if (correct_blank && buf->outptr != buf->buffer) {
        if (buf->outptr[-1] !=' ' && buf->outptr[-1] != ']') {
            if (!_can_push(buf)) return 0;
            *buf->outptr++=' ';
        }
    }
    for (;*c;) {
        if (!_can_push(buf)) return 0;
        *buf->outptr++ = *c++;
    }
    return 1;
}

static int _pushout(struct microlena_Buffer *buf, char c)
{
    if (!_can_push(buf)) return 0;
    *buf->outptr++ = c;
    return 1;
}


void microlena_ShiftBuffer(struct microlena_Buffer *buf)
{
    if (buf->readmode) {
        buf->inbufptr = buf->inptr;
        buf->readmode = 0;
    }
    int d = strlen(buf->buffer)+1;
    buf->inptr = buf->buffer + buf->bufsize - d;
    memmove(buf->inptr, buf->buffer, d);
    buf->outptr = buf->buffer;
}

void microlena_StartTextBuffer(struct microlena_Buffer *buf)
{
    buf->readmode=1;
    buf->inptr = buf->inbufptr;
    buf->outptr = buf->buffer;
}

int microlena_TranslatePhrase(struct microlena_Buffer *buf)
{
    
    microlena_StartTextBuffer(buf);
    if (buf->show_phonemes) printf("Input %s\n", buf->inbufptr);
    int m=microlena_Phraser(buf);
    if (buf->show_phonemes) printf("Frazer %d, w buforze <%s>\n", m, buf->inptr);
    if (m <= 0) return m;
    m=microlena_PreStress(buf);
    if (buf->show_phonemes) printf("Prestresser %d, w buforze <%s>\n", m, buf->buffer);
    if (m <= 0) return m;
    microlena_ShiftBuffer(buf);
    m=microlena_Translate(buf);
    if (buf->show_phonemes) printf("Translator %d, w buforze <%s>\n", m, buf->buffer);
    if (m <= 0) return m;
    microlena_ShiftBuffer(buf);
    m=microlena_Poststresser(buf);
    if (buf->show_phonemes) printf("Poststresser %d, w buforze <%s>\n", m, buf->buffer);
    if (m <= 0) return m;
    microlena_ShiftBuffer(buf);
    m=microlena_Intonator(buf);
    return m;
}

struct microlena_Buffer *microlena_InitBuffer(const char *text, int convert)
{
    struct microlena_Buffer *buf;
    int d;
    if (convert) {
        d= microlena_utf2iso(text, NULL);
    }
    else {
        d=strlen(text)+1;
    }
    
    buf=calloc(sizeof(struct microlena_Buffer) + 4096 - 4+d, 1);
    buf->bufsize = 4096;
    buf->margin = 128;
    buf->inbuf = buf->buffer + 4096;
    if (convert) {
        microlena_utf2iso(text, buf->inbuf);
    }
    else {
        strcpy(buf->inbuf, text);
    }
    buf->outptr = buf->inptr = buf->buffer;
    *buf->inptr=0;
    buf->inbufptr = buf->inbuf;
    buf->_pushout = _pushout;
    buf->_pushstr = _pushstr;
    buf->ft = 1;
    strcpy(buf->decipoint,"przecinek");
    return buf;
}

void microlena_simpleDeciPoint(struct microlena_Buffer *buf, int dp)
{
    buf -> simple_decipoint = dp ? 1 : 0;
}

int microlena_setDeciPoint(struct microlena_Buffer *buf, char *c)
{
    if (strlen(c) > 1 && strlen(c) < 16) {
        strcpy(buf->decipoint,c);
        return 0;
    }
    return -1;
}

int microlena_getPhonemes(struct microlena_Buffer *buf, char *outbuf)
{
    int m;
    for (;;) {
        if (buf -> ft) {
            buf->ft=0;
            strcpy(outbuf,"_ 5");
            m=microlena_TranslatePhrase(buf);
            if (m <=0 ) return m;
            return 1;
        }
        m = microlena_getMbrolaPhone(buf, outbuf);
        if (m) return m;
        m=microlena_TranslatePhrase(buf);
        if (m <=0 ) return m;
    }
    return -1;
}



static const char *a_string[]={
/* 0a0 */ "\xa0",
/* 0a1 */ "!",
/* 0a2 */ " cent ",
/* 0a3 */ " funt ",
/* 0a4 */ "\xa4",
/* 0a5 */ " jen ",
/* 0a6 */ "|",
/* 0a7 */ "\xa7",
/* 0a8 */ " ",
/* 0a9 */ " copyright ",
/* 0aa */ " ",
/* 0ab */ "\"",
/* 0ac */ "~",
/* 0ad */ "\xad",
/* 0ae */ " registered ",
/* 0af */ "-",
/* 0b0 */ "\xb0",
/* 0b1 */ " plus minus ",
/* 0b2 */ "^2",
/* 0b3 */ "^3",
/* 0b4 */ " ",
/* 0b5 */ "mikro",
/* 0b6 */ " ",
/* 0b7 */ ".",
/* 0b8 */ " ",
/* 0b9 */ "^1",
/* 0ba */ " ",
/* 0bb */ "\"",
/* 0bc */ " 1/4 ",
/* 0bd */ " 1/2 ",
/* 0be */ " 3/4 ",
/* 0bf */ "?",
/* 0c0 */ "\xc1",
/* 0c1 */ "\xc1",
/* 0c2 */ "\xc2",
/* 0c3 */ "\xc2",
/* 0c4 */ "\xc4",
/* 0c5 */ "\xc3", // "AA",
/* 0c6 */ "\xcb", // "AE",
/* 0c7 */ "\xc7",
/* 0c8 */ "\xc9",
/* 0c9 */ "\xc9",
/* 0ca */ "E",
/* 0cb */ "\xcb",
/* 0cc */ "I",
/* 0cd */ "\xcd",
/* 0ce */ "\xce",
/* 0cf */ "I",
/* 0d0 */ "\xd0",
/* 0d1 */ "\xd2",
/* 0d2 */ "O",
/* 0d3 */ "\xd3",
/* 0d4 */ "\xd4",
/* 0d5 */ "\xd5",
/* 0d6 */ "\xd6",
/* 0d7 */ "\xd7",
/* 0d8 */ "\xd6",
/* 0d9 */ "\xda",
/* 0da */ "\xda",
/* 0db */ "U",
/* 0dc */ "\xdc",
/* 0dd */ "\xdd",
/* 0de */ "T", //"TH",
/* 0df */ "\xdf",
/* 0e0 */ "\xe1",
/* 0e1 */ "\xe1",
/* 0e2 */ "\xe2",
/* 0e3 */ "\xe2",
/* 0e4 */ "\xe4",
/* 0e5 */ "\xe3", //"aa",
/* 0e6 */ "\xe4", //"ae",
/* 0e7 */ "\xe7",
/* 0e8 */ "\xe9",
/* 0e9 */ "\xe9",
/* 0ea */ "e",
/* 0eb */ "\xeb",
/* 0ec */ "\xed",
/* 0ed */ "\xed",
/* 0ee */ "\xee",
/* 0ef */ "i",
/* 0f0 */ "d", //"dh",
/* 0f1 */ "\xf2",
/* 0f2 */ "o",
/* 0f3 */ "\xf3",
/* 0f4 */ "\xf4",
/* 0f5 */ "\xf5",
/* 0f6 */ "\xf6",
/* 0f7 */ "\xf7",
/* 0f8 */ "\xf6",
/* 0f9 */ "\xfa",
/* 0fa */ "\xfa",
/* 0fb */ "u",
/* 0fc */ "\xfc",
/* 0fd */ "\xfd",
/* 0fe */ "t", //"th",
/* 0ff */ "y",
/* 100 */ "A",
/* 101 */ "a",
/* 102 */ "\xc3",
/* 103 */ "\xe3",
/* 104 */ "\xa1",
/* 105 */ "\xb1",
/* 106 */ "\xc6",
/* 107 */ "\xe6",
/* 108 */ "C",
/* 109 */ "c",
/* 10a */ "C",
/* 10b */ "c",
/* 10c */ "\xc8",
/* 10d */ "\xe8",
/* 10e */ "\xcf",
/* 10f */ "\xef",
/* 110 */ "\xd0",
/* 111 */ "\xf0",
/* 112 */ "E",
/* 113 */ "e",
/* 114 */ "E",
/* 115 */ "e",
/* 116 */ "E",
/* 117 */ "e",
/* 118 */ "\xca",
/* 119 */ "\xea",
/* 11a */ "\xcc",
/* 11b */ "\xec",
/* 11c */ "G",
/* 11d */ "g",
/* 11e */ "G",
/* 11f */ "g",
/* 120 */ "G",
/* 121 */ "g",
/* 122 */ "G",
/* 123 */ "g",
/* 124 */ "H",
/* 125 */ "h",
/* 126 */ "H",
/* 127 */ "H",
/* 128 */ "I",
/* 129 */ "i",
/* 12a */ "I",
/* 12b */ "i",
/* 12c */ "I",
/* 12d */ "i",
/* 12e */ "I",
/* 12f */ "i",
/* 130 */ "I",
/* 131 */ "i",
/* 132 */ "IJ",
/* 133 */ "ij",
/* 134 */ "J",
/* 135 */ "j",
/* 136 */ "K",
/* 137 */ "k",
/* 138 */ "k",
/* 139 */ "\xc5",
/* 13a */ "\xe5",
/* 13b */ "L",
/* 13c */ "l",
/* 13d */ "\xa5",
/* 13e */ "\xb5",
/* 13f */ "L",
/* 140 */ "l",
/* 141 */ "\xa3",
/* 142 */ "\xb3",
/* 143 */ "\xd1",
/* 144 */ "\xf1",
/* 145 */ "N",
/* 146 */ "n",
/* 147 */ "\xd2",
/* 148 */ "\xf2",
/* 149 */ "'n",
/* 14a */ "NG",
/* 14b */ "ng",
/* 14c */ "O",
/* 14d */ "o",
/* 14e */ "O",
/* 14f */ "o",
/* 150 */ "\xd5",
/* 151 */ "\xf5",
/* 152 */ "\xd6",//"OE",
/* 153 */ "\xf6",//"oe",
/* 154 */ "\xc0",
/* 155 */ "\xe0",
/* 156 */ "R",
/* 157 */ "r",
/* 158 */ "\xd8",
/* 159 */ "\xf8",
/* 15a */ "\xa6",
/* 15b */ "\xb6",
/* 15c */ "S",
/* 15d */ "s",
/* 15e */ "\xaa",
/* 15f */ "\xba",
/* 160 */ "\xa9",
/* 161 */ "\xb9",
/* 162 */ "\xde",
/* 163 */ "\xfe",
/* 164 */ "\xab",
/* 165 */ "\xbb",
/* 166 */ "T",
/* 167 */ "t",
/* 168 */ "U",
/* 169 */ "u",
/* 16a */ "U",
/* 16b */ "u",
/* 16c */ "U",
/* 16d */ "u",
/* 16e */ "\xd9",
/* 16f */ "\xf9",
/* 170 */ "\xdb",
/* 171 */ "\xfb",
/* 172 */ "U",
/* 173 */ "u",
/* 174 */ "W",
/* 175 */ "w",
/* 176 */ "Y",
/* 177 */ "y",
/* 178 */ "Y",
/* 179 */ "\xac",
/* 17a */ "\xbc",
/* 17b */ "\xaf",
/* 17c */ "\xbf",
/* 17d */ "\xae",
/* 17e */ "\xbe",
/* 17f */ "s"};


static int uni_isspace(int znak)
{
	int i;
	static int spacje[]={0x1680,0x180e,0x2028,0x2029,0x205f,0x3000,0};
	if (znak<=0x20) return 1;
	if (znak>=0x7f && znak <=0xa0) return 1;
	if (znak>=0x2000 && znak <=0x200b) return 1;
	for (i=0;spacje[i];i++) if (znak==spacje[i]) return 1;
	return 0;
}

#define pushout_u(n) do {if (outstr) outstr[pos]=n;pos++;} while (0)

static int get_unichar(const char **str)
{
	int znak,n,m;
	if (!*str) return 0;
	znak=(*(*str)++) & 255;
	if (!(znak & 0x80)) return znak;
	if ((znak & 0xe0)==0xc0) n=1;
	else if ((znak & 0xf0)==0xe0) n=2;
	else {
		return 0;
	}
	znak &= 0x1f;
	while (n--) {
		m=*(*str)++ & 255;
		if ((m & 0xc0)!=0x80) {
			return 0;
		}
		znak=(znak<<6) | (m & 0x3f);
	}
	return znak;
}

static const struct {
	int znak;
	const char *repr;
} prochar[]={
{0x2116," numer "},
{0x2122," tm "},
{0x2126,"ohm "},
{0x3a9,"ohm "},
{0,NULL}};

int microlena_utf2iso(const char *instr,char *outstr)
{
	int pos=0;
	int znak,i;
	while (*instr) {
		const char *c=instr;
		znak=get_unichar(&c);
		if (!uni_isspace(znak)) break;
		instr=c;
	}
	if (!*instr) {
		
		return 0;
	}
	while (*instr) {
		znak=get_unichar(&instr);
		if (znak=='\r') {
			if (*instr=='\n') instr++;
			pushout_u('\n');
			continue;
		}
		if (znak=='\n') {
			pushout_u('\n');
			continue;
		}
		if (uni_isspace(znak)) {
			pushout_u(' ');
			continue;
		}
		if (znak==',' && *instr==',') {
			instr++;
			pushout_u('"');
			continue;
		}
		if (znak <0x80) {
			pushout_u(znak);
			continue;
		}
		if (znak == 0x2022) {
			pushout_u('*');
			continue;
		}
		if (znak == 0x2026) {
			pushout_u('.');
			pushout_u('.');
			pushout_u('.');
			continue;
		}
		if (znak==0x218) znak=0x15e;
		else if (znak==0x219) znak=0x15f;
		else if (znak==0x21a) znak=0x162;
		else if (znak==0x21b) znak=0x163;
		if (znak<=0x17f) {
			const char *d=a_string[znak-0xa0];
			while (*d) {
				pushout_u(*d);
				d++;
			}
			continue;
		}
		if (znak >= 0x2018 && znak <=0x201b) {
			pushout_u('\'');
			continue;
		}
		if ((znak >= 0x201c && znak <=0x201f) || znak==0x2039 || znak==0x203a) {
			pushout_u('"');
			continue;
		}
		if (znak== 0x2013 || znak == 0x2014 || znak == 0x2212 || znak == 0x2015) {
			pushout_u('-');
			continue;
		}
		for (i=0;prochar[i].znak;i++) if (prochar[i].znak == znak) break;
		if (prochar[i].znak) {
			const char *c=prochar[i].repr;
			for (;*c;c++) {
				pushout_u(*c);
			}
			continue;
		}
        break;
	}
	pushout_u(0);
	return pos;
}
#undef pushout_u
