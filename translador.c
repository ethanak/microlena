#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <ctype.h>
#include "milv_datatypes.h"

//#define DUG 1

extern const char *const pho_declares[];
extern const uint16_t pho_rule_array[];
extern const int pho_ruleset_cnt;
extern const struct binlet_ruleset pho_ruleset[];
extern const struct binlet_rule binlet_rules[];

static char *next_char(char *c, int *sep)
{
    if (!*c) return c;
    c++;
    if (sep) *sep = 0;
    for (;;) {
        if (*c == '~') {        /* stress etc. control sequences */
            c++;
            if (*c == '\'' && sep) *sep = 1;
            else if (*c == '+' && sep) *sep = 2;
            if (*c) c++;
            continue;
        }
        if (*c == '[') {        /* extra informations about word */
            c = strchr((char *)c, ']');
            if (!c) return (char *)"";
            c++;
            continue;
        }
        return c;
    }
}

static char *prev_char(char *c, char *start)
{
    char *d;

    if (!c) return 0;
    if (c <= start) return 0;
    c--;
    while (c >= start) {
        if (c > start) {
            d = c - 1;
            if (*d == '~') {
                c = d - 1;
                if (c < start) return 0;
                continue;
            }
        }
        if (*c == '}') {
            c--;
            while (c >= start && c[1] != '{') c--;
            if (c < start) return 0;
            continue;
        }
        if (*c == ']') {
            c--;
            while (c >= start && c[1] != '[') c--;
            if (c < start) return 0;
            continue;
        }
        return c;
    }
    if (c < start) return 0;
    return c;
}

#define OP_CLASS 0x100
#define OP_CHAR 0x200
#define OP_NEG 0x400
#define OP_OR 0x800
#define OP_SEP 0x1000
#define OP_STAR 0x2000
#define OP_MOD 0x4000
#define OP_NEGCLASS 0x8000

static char *dopasuj_r(char *str, uint16_t rule, int sep)
{
    int nr, op;

    for (; rule != 0xffff; rule = pho_rule_array[rule + 1]) {
        op = pho_rule_array[rule];
        if (op & OP_STAR) {
            char *c;
            for (;;) {
                if (!str || !*str || microlena_isspace(*str)) return NULL;
                c = dopasuj_r(str, pho_rule_array[rule + 2], 0);
                if (c) {
                    break;
                }
                str = next_char(str, NULL);
            }
            continue;
        }
        if (op & OP_SEP) {
            if (sep != 1) return NULL;
            continue;
        }
        if (op & OP_MOD) {
            if (sep != 2) return NULL;
            continue;
        }
        if (op & OP_CHAR) {
            nr = op & 255;
#ifdef DUG
            printf("Dopasowuje '%s' do %c\n", str, nr);
#endif
            if (nr == '$') {
                if (*str) {
                    return NULL;
                }
#ifdef DUG
                printf("Pasi\n");
#endif
                str = next_char(str, NULL);
                continue;
            }
            if (nr == '#') {
                if (*str && !microlena_isspace(*str)) {
                    return NULL;
                }
#ifdef DUG
                printf("Pasi\n");
#endif
                str = next_char(str, NULL);
                continue;
            }
            if ((*str & 255) != nr) {
                return NULL;
            }
#ifdef DUG
            printf("Pasi\n");
#endif
            str = next_char(str, NULL);
            continue;
        }
        if (op & OP_OR) {
            char *c;
            c = dopasuj_r(str, pho_rule_array[rule + 2], sep);
            if (!c) c = dopasuj_r(str, pho_rule_array[rule + 3], sep);
            if (!c) return NULL;
            str = c;
            continue;
        }
        if (op & OP_CLASS) {

            nr = (op & 255) - 'A';
#ifdef DUG
            printf("Dopasowuje klase %c(%s) do %s\n", nr + 'A',
                   pho_declares[nr], str);
#endif
            if (!*str) return NULL;
            if (pho_declares[nr] && strchr(pho_declares[nr], *str)) {
                str = next_char(str, NULL);
#ifdef DUG
                printf("Pasi\n");
#endif
                continue;
            }
            return NULL;
        }
        if (op & OP_NEGCLASS) {
            if (!*str) return NULL;
            nr = (op & 255) - 'A';
#ifdef DUG
            printf("Dopasowuje antyklase %c(%s) do %s\n", nr + 'A',
                   pho_declares[nr], str);
#endif
            if (pho_declares[nr] && strchr(pho_declares[nr], *str)) {
                return NULL;
            }
#ifdef DUG
            printf("Pasi\n");
#endif
            str = next_char(str, NULL);
            continue;
        }
        if (op & OP_NEG) {
            char *s;
            nr = op & 255;
            if (!*str) return NULL;
            if (nr) {
                nr -= 'A';
#ifdef DUG
                printf("Dopasowuje klase roznicowa %c(%s) do %s\n", nr + 'A',
                       pho_declares[nr], str);
#endif
                if (!pho_declares[nr] || !strchr(pho_declares[nr], *str)) {
                    return NULL;
                }
            }
#ifdef DUG
            printf("Dopasowuje brak %s w %s\n",
                   microlena_LocString(pho_rule_array[rule + 2]), str);
#endif
            if (strchr(microlena_LocString(pho_rule_array[rule + 2]), *str)) {
                return NULL;
            }
#ifdef DUG
            printf("Pasi\n");
#endif
            str = next_char(str, NULL);
            continue;
        }
        return NULL;            // error?
    }
    return str;
}

static int dopasuj_l(char *str, uint16_t rule, char *start, char **cout)
{
    int nr, op;
    for (; rule != 0xffff; rule = pho_rule_array[rule + 1]) {
        op = pho_rule_array[rule];
        if (!str) {
            if (cout) *cout = NULL;
            return ((op == (OP_CHAR | '#')) || (op == (OP_CHAR | '^'))) ? 1 : 0;
        }
        //return 0;

        if (op & OP_STAR) {
            int n;
            char *c;
            if (pho_rule_array[rule + 2] != 0xffff) {
                n = dopasuj_l(str, pho_rule_array[rule + 2], start,
                              (char **)&c);
                if (!n) return 0;
                str = c;
            }
            for (;;) {
                if (!str || !*str || microlena_isspace(*str)) return 0;
                n = dopasuj_l(str, pho_rule_array[rule + 1], start,
                              (char **)&c);
                if (n) {
                    str = c;
                    continue;
                }
                str = prev_char(str, start);
            }
            return 0;
        }
        if (op & OP_CHAR) {
            nr = op & 255;
            // ^ już sprawdzony
            if (nr == '#') {
                if (*str && !microlena_isspace(*str)) return 0;
                str = prev_char(str, start);
                continue;
            }
            if ((*str & 255) != nr) return 0;
            str = prev_char(str, start);
            continue;
        }
        if (op & OP_OR) {
            char *c;
            int n;
            n = dopasuj_l(str, pho_rule_array[rule + 2], start, (char **)&c);
            if (!n) {
                n = dopasuj_l(str, pho_rule_array[rule + 3], start,
                              (char **)&c);
            }
            if (!n) {
                return 0;
            }
            str = c;
            continue;
        }
        if (op & OP_CLASS) {
            nr = (op & 255) - 'A';
            if (!*str) return 0;
            if (pho_declares[nr] && strchr(pho_declares[nr], *str)) {
                str = prev_char(str, start);
                continue;
            }
            return 0;
        }
        if (op & OP_NEGCLASS) {
            nr = (op & 255) - 'A';
            if (!*str) return 0;
            if (pho_declares[nr] && strchr(pho_declares[nr], *str)) {
                return 0;
            }
            str = prev_char(str, start);
            continue;
        }
        if (op & OP_NEG) {
            if (!*str) return 0;
            if (op & 255) {
                nr = (op & 255) - 'A';
                if (!pho_declares[nr] || !strchr(pho_declares[nr], *str)) {
                    return 0;
                }
            }
            char *s;
            for (s = microlena_LocString(pho_rule_array[rule + 2]); *s; s++) {
                if (*s == '#') {
                    if (microlena_isspace(*str)) return 0;
                    continue;
                }
                if (*s == '^') {
                    if (!str) return 0;
                    continue;
                }
                if (str && (*str & 255) == (*s & 255)) return 0;
            }
            str = prev_char(str, start);
        }
        return 0;               /* error? */
    }
    if (cout) *cout = str;
    return 1;
}

static void dump_rule(uint16_t rule)
{
    char *c;
    int op;
    for (; rule != 0xffff; rule = pho_rule_array[rule + 1]) {
        op = pho_rule_array[rule];
        switch (op & 0xff00) {
        case OP_NEGCLASS:
            printf("!");
        case OP_CLASS:
        case OP_CHAR:
            printf("%c", op & 0xff);
            break;

        case OP_NEG:
            printf("(!");
            if (op & 0xff) printf("%c", op & 0xff);
            printf("%s)", microlena_LocString(pho_rule_array[rule + 2]));
            break;

        case OP_SEP:
        case OP_MOD:
            printf("?");
            break;

        case OP_STAR:
            printf("*");
            //if (pho_rule_array[rule+2] != 0xffff) printf("%s",microlena_LocString(pho_rule_array[rule+2]));
            break;
        case OP_OR:
            printf("(");
            dump_rule(pho_rule_array[rule + 2]);
            printf(",");
            dump_rule(pho_rule_array[rule + 3]);
            printf(")");
            break;
        }
    }
}

static char *dopasuj_rule(char *str, char *start, int sep,
                          const struct binlet_rule *rule, int *eat)
{
    char *cstr = next_char(str, &sep);
    char *lstr = prev_char(str, start);
    if (rule->left != 0xffff) {
        //return NULL;
#ifdef DUG
        printf("L: %04x\n", rule->left);
        dump_rule(rule->left);
        printf("\n");
        printf("Test <%p>\n", lstr);
#endif
        if (!dopasuj_l(lstr, rule->left, start, NULL)) return NULL;
#ifdef DUG
        printf("Lewa pasuje\n");
#endif
    }
    if (rule->right != 0xffff) {
#ifdef DUG

        printf("R %04x:\n", rule->right);
        dump_rule(rule->right);
        printf(" -> %s\n", microlena_LocString(rule->trans));
#endif
        if (!dopasuj_r(cstr, rule->right, sep)) return NULL;
#ifdef DUG
        printf("Prawa pasuje do %s\n", cstr);
#endif
    }
#ifdef DUG

    printf("Returns %d:%s\n", rule->eat, microlena_LocString(rule->trans));
#endif
    *eat = rule->eat;
    return microlena_LocString(rule->trans);
}

static char *dopasuj_litera(char *str, char *start, int sep, int *eat)
{
    int i;
    const struct binlet_ruleset *rs;

        for (i = 0; i < pho_ruleset_cnt;
         i++) if (pho_ruleset[i].letter == *(uint8_t *) str) break;
//    if (i>=pho_ruleset_cnt) return NULL;
    if (i >= pho_ruleset_cnt) {
        if (eat) *eat = 0;
        return " ";
    }
#ifdef DUG
    printf("Litera %c\n", *str);
#endif
    rs = &pho_ruleset[i];
    uint16_t fr = rs->first_rule;
    for (i = 0; i < rs->rule_count; i++) {
        char *c = dopasuj_rule(str, start, sep, &binlet_rules[fr + i], eat);
        if (c) return c;
    }
#ifdef DUG
    printf("Default %s\n", microlena_LocString(rs->deftrans));
#endif
    *eat = 0;                   // brak eat dla defaultów
    return microlena_LocString(rs->deftrans);

}

static void reschwa(char *c)
{
    int e = 0;
    for (; *c; c++) {
        if (*c == 'e' || *c == 'E') {
            e = 1;
            continue;
        }
        if (strchr("aiouyOE", *c)) {
            e = 0;
            continue;
        }
        if (*c == 'Y' && e) *c = 'I';
    }
}

int microlena_Translate(struct microlena_Buffer *buf)
{
    int eats = 0;
    char *start, *str, *outstart;
    if (microlena_isdigit(*buf->inptr) && buf->inptr[1] == ':') {
        pushout(buf, *buf->inptr++);
        pushout(buf, *buf->inptr++);
    }
    while (*buf->inptr && microlena_isspace(*buf->inptr)) buf->inptr++;
    start = str = buf->inptr;
    outstart = buf->outptr;
    for (;;) {
        //printf("AS %s\n",str);
        while (*str && microlena_isspace(*str)) str++;
        if (!*str) break;
        blank(buf);
        //printf("BS %s\n",str);
        if (*str == '[') {
            while (*str) {
                pushout(buf, *str);
                if (*str++ == ']') break;
            }
        }
        while (*str == '~') {
            if (str[1] == '\'' || str[1] == '+') {
                str += 2;
            }
            pushout(buf, '~');
            str++;
            if (*str) {
                pushout(buf, *str++);
            }
        }
        int sep = 0;
        int eat;
        for (; str && *str && !microlena_isspace(*str);) {
            //printf("STT %s\n",str);
            char *c = dopasuj_litera(str, start, sep, &eat);
            if (!c) break;
            pushstr(buf, c, 0);
            str = next_char(str, &sep);
            while (eat-- > 0) str = next_char(str, &sep);
        }
    }
    pushout(buf, 0);
    reschwa(outstart);

    return 1;
}
