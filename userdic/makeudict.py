#!/usr/bin/env python3
#coding: utf-8

import re, sys

args=sys.argv[1:]
dicname = args.pop(0)
uname = 'userdia'
outname = None
defines = False

def error(s):
    sys.stderr.write(s+'\n')
    exit(1)

while args:
    d=args.pop(0)
    if d == '-name':
        uname = args.pop(0)
    elif d == '-out':
        outname = args.pop(0)
    elif d == '-defs':
        defines = True
    else:
        error("Nieznany parametr %s" % d)

a_string=(
    b"\xa0",    # 0a0 
    b"!",    # 0a1 
    b" cent ",    # 0a2 
    b" funt ",    # 0a3 
    b"\xa4",    # 0a4 
    b" jen ",    # 0a5 
    b"|",    # 0a6 
    b"\xa7",    # 0a7 
    b" ",    # 0a8 
    b" copyright ",    # 0a9 
    b" ",    # 0aa 
    b"\"",    # 0ab 
    b"~",    # 0ac 
    b"\xad",    # 0ad 
    b" registered ",    # 0ae 
    b"-",    # 0af 
    b"\xb0",    # 0b0 
    b" plus minus ",    # 0b1 
    b"^2",    # 0b2 
    b"^3",    # 0b3 
    b" ",    # 0b4 
    b"mikro",    # 0b5 
    b" ",    # 0b6 
    b".",    # 0b7 
    b" ",    # 0b8 
    b"^1",    # 0b9 
    b" ",    # 0ba 
    b"\"",    # 0bb 
    b" 1/4 ",    # 0bc 
    b" 1/2 ",    # 0bd 
    b" 3/4 ",    # 0be 
    b"?",    # 0bf 
    b"\xc1",    # 0c0 
    b"\xc1",    # 0c1 
    b"\xc2",    # 0c2 
    b"\xc2",    # 0c3 
    b"\xc4",    # 0c4 
    b"\xc3", # "AA",    # 0c5 
    b"\xcb", # "AE",    # 0c6 
    b"\xc7",    # 0c7 
    b"\xc9",    # 0c8 
    b"\xc9",    # 0c9 
    b"E",    # 0ca 
    b"\xcb",    # 0cb 
    b"I",    # 0cc 
    b"\xcd",    # 0cd 
    b"\xce",    # 0ce 
    b"I",    # 0cf 
    b"\xd0",    # 0d0 
    b"\xd2",    # 0d1 
    b"O",    # 0d2 
    b"\xd3",    # 0d3 
    b"\xd4",    # 0d4 
    b"\xd5",    # 0d5 
    b"\xd6",    # 0d6 
    b"\xd7",    # 0d7 
    b"\xd6",    # 0d8 
    b"\xda",    # 0d9 
    b"\xda",    # 0da 
    b"U",    # 0db 
    b"\xdc",    # 0dc 
    b"\xdd",    # 0dd 
    b"T", #"TH",    # 0de 
    b"\xdf",    # 0df 
    b"\xe1",    # 0e0 
    b"\xe1",    # 0e1 
    b"\xe2",    # 0e2 
    b"\xe2",    # 0e3 
    b"\xe4",    # 0e4 
    b"\xe3", #"aa",    # 0e5 
    b"\xe4", #"ae",    # 0e6 
    b"\xe7",    # 0e7 
    b"\xe9",    # 0e8 
    b"\xe9",    # 0e9 
    b"e",    # 0ea 
    b"\xeb",    # 0eb 
    b"\xed",    # 0ec 
    b"\xed",    # 0ed 
    b"\xee",    # 0ee 
    b"i",    # 0ef 
    b"d", #"dh",    # 0f0 
    b"\xf2",    # 0f1 
    b"o",    # 0f2 
    b"\xf3",    # 0f3 
    b"\xf4",    # 0f4 
    b"\xf5",    # 0f5 
    b"\xf6",    # 0f6 
    b"\xf7",    # 0f7 
    b"\xf6",    # 0f8 
    b"\xfa",    # 0f9 
    b"\xfa",    # 0fa 
    b"u",    # 0fb 
    b"\xfc",    # 0fc 
    b"\xfd",    # 0fd 
    b"t", #"th",    # 0fe 
    b"y",    # 0ff 
    b"A",    # 100 
    b"a",    # 101 
    b"\xc3",    # 102 
    b"\xe3",    # 103 
    b"\xa1",    # 104 
    b"\xb1",    # 105 
    b"\xc6",    # 106 
    b"\xe6",    # 107 
    b"C",    # 108 
    b"c",    # 109 
    b"C",    # 10a 
    b"c",    # 10b 
    b"\xc8",    # 10c 
    b"\xe8",    # 10d 
    b"\xcf",    # 10e 
    b"\xef",    # 10f 
    b"\xd0",    # 110 
    b"\xf0",    # 111 
    b"E",    # 112 
    b"e",    # 113 
    b"E",    # 114 
    b"e",    # 115 
    b"E",    # 116 
    b"e",    # 117 
    b"\xca",    # 118 
    b"\xea",    # 119 
    b"\xcc",    # 11a 
    b"\xec",    # 11b 
    b"G",    # 11c 
    b"g",    # 11d 
    b"G",    # 11e 
    b"g",    # 11f 
    b"G",    # 120 
    b"g",    # 121 
    b"G",    # 122 
    b"g",    # 123 
    b"H",    # 124 
    b"h",    # 125 
    b"H",    # 126 
    b"H",    # 127 
    b"I",    # 128 
    b"i",    # 129 
    b"I",    # 12a 
    b"i",    # 12b 
    b"I",    # 12c 
    b"i",    # 12d 
    b"I",    # 12e 
    b"i",    # 12f 
    b"I",    # 130 
    b"i",    # 131 
    b"IJ",    # 132 
    b"ij",    # 133 
    b"J",    # 134 
    b"j",    # 135 
    b"K",    # 136 
    b"k",    # 137 
    b"k",    # 138 
    b"\xc5",    # 139 
    b"\xe5",    # 13a 
    b"L",    # 13b 
    b"l",    # 13c 
    b"\xa5",    # 13d 
    b"\xb5",    # 13e 
    b"L",    # 13f 
    b"l",    # 140 
    b"\xa3",    # 141 
    b"\xb3",    # 142 
    b"\xd1",    # 143 
    b"\xf1",    # 144 
    b"N",    # 145 
    b"n",    # 146 
    b"\xd2",    # 147 
    b"\xf2",    # 148 
    b"'n",    # 149 
    b"NG",    # 14a 
    b"ng",    # 14b 
    b"O",    # 14c 
    b"o",    # 14d 
    b"O",    # 14e 
    b"o",    # 14f 
    b"\xd5",    # 150 
    b"\xf5",    # 151 
    b"\xd6",#OE",    # 152 
    b"\xf6",#"oe",    # 153 
    b"\xc0",    # 154 
    b"\xe0",    # 155 
    b"R",    # 156 
    b"r",    # 157 
    b"\xd8",    # 158 
    b"\xf8",    # 159 
    b"\xa6",    # 15a 
    b"\xb6",    # 15b 
    b"S",    # 15c 
    b"s",    # 15d 
    b"\xaa",    # 15e 
    b"\xba",    # 15f 
    b"\xa9",    # 160 
    b"\xb9",    # 161 
    b"\xde",    # 162 
    b"\xfe",    # 163 
    b"\xab",    # 164 
    b"\xbb",    # 165 
    b"T",    # 166 
    b"t",    # 167 
    b"U",    # 168 
    b"u",    # 169 
    b"U",    # 16a 
    b"u",    # 16b 
    b"U",    # 16c 
    b"u",    # 16d 
    b"\xd9",    # 16e 
    b"\xf9",    # 16f 
    b"\xdb",    # 170 
    b"\xfb",    # 171 
    b"U",    # 172 
    b"u",    # 173 
    b"W",    # 174 
    b"w",    # 175 
    b"Y",    # 176 
    b"y",    # 177 
    b"Y",    # 178 
    b"\xac",    # 179 
    b"\xbc",    # 17a 
    b"\xaf",    # 17b 
    b"\xbf",    # 17c 
    b"\xae",    # 17d 
    b"\xbe",    # 17e 
    b"s")    # 17f 

prochar={
0x2116:b" numer ",
0x2122:b" tm ",
0x2126:b"ohm ",
0x3a9:b"ohm ",
0x2022:b'*',
0x2026:b'...'
}

vznak={
0x218:0x15e,
0x219:0x15f,
0x21a:0x162,
0x21b:0x163}
        

def repchr(ucode):
    if ucode == '\n':
        return b'\n'
    if ucode <= 0x20:
        return b' '
    if ucode < 0x7f:
        return bytes([ucode])
    if ucode < 0xa0:
        return b' '
    ucode = vznak.get(ucode, ucode)
    if ucode < 0x180:
        return a_string[ucode-0xa0]
    if ucode>=0x2000 and ucode <=0x200b:
        return b' '
    if ucode in (0x1680, 0x180e, 0x2028, 0x2029, 0x205f, 0x3000):
        return b' '
    if ucode in prochar:
        return prochar[ucode]
    if ucode >= 0x2018 and ucode <=0x201b:
        return b"'"
    if (ucode >= 0x201c and ucode <=0x201f) or ucode in (0x2039,0x203a):
        return b'"'
    if ucode in (0x2013, 0x2014, 0x2212, 0x2015):
        return b'-';
    return b' '

def repstr(napis):
    napis = re.sub(r'[\r\n]+','\n',napis, re.DOTALL).replace(',,','"')
    return b''.join(repchr(ord(a)) for a in napis)

units = []
lines = []


rcs=re.compile(r"^[a-ząęśćńóźżł](~')?(_?[a-z@ąęśćńóźżł](~')?)*$")

rcw = re.compile(r"^((%[1-4]?)|([a-ząęśćńóźżł](~')?))(_?(([a-z@ąęśćńóźżł](~')?)|@|%[1-4]?))*$")


def proper_word(word, isline):
    r=re.match(r'\[(.*?)\](.*)$', word)
    if r:
        u=False
        s=False
        word = r.group(2)
        t=r.group(1)
        for a in t:
            if a == 'u':
                if u:
                    return False
                u=True
                continue
            if a in "1234":
                if s:
                    return False
                s=True
                continue
            return False
    if isline:
        if not rcw.match(word):
            return False
    else:
        if not rcs.match(word):
            return False
    return True
    
def proper_str(s, isline):
    words = s.split()
    for word in words:
        if not proper_word(word, isline):
            return False
    return True
    
def addunit(line, female):
    line = line.split(None,1)
    if len(line) != 2:
        error('Błędna jednostka %s' % line[0])
    u=line[0]
    spc = line[1].split('|')
    if len(spc) != 4:
        error('Błędne rozwinięcie jednostki %s' % line[1])
    spc=list(re.sub(r'\s+',' ',x.strip()) for x in spc)
    for part in spc:
        if not proper_str(part, False):
            error('Błędny zapis %s' % part)
    units.append(female + repstr(u) + b'\0'+repstr('|'.join(spc)))

def addhash(line):
    line=line.split(None, 1)
    if len(line) != 2:
        error('Błędne polecenie #%s' % line[0])
    if line[0] == 'unit':
        return addunit(line[1],b'm')
    if line[0] == 'unitf':
        return addunit(line[1],b'f')
    error('Błędne polecenie #%s' % line[0])
    

UDIF_ATEND=1
UDIF_ABBR=2
UDIF_UNSTR=4
UDIF_COPY=8

def asorta(s):
    s=s.group(1).split('|')
    s.sort(key=len, reverse = True)
    return '(%s)' % ('|'.join(s))
    
    
def asort(patr):
    return re.sub(r'\(([^)]+)\)', asorta, patr)

def add_udic(linein):
    line=linein.split(None,1)
    if len(line) != 2:
        error('Błędny zapis: %s' % linein)
    pattern = line[0]
    result = line[1]
    flags = ''
    iflags = 128
    r=re.match(r'^(.*)\$(.*)$', result)
    if r:
        result=r.group(1).strip()
        flags = r.group(2)
        if flags == 'S':
            if result:
                error('Błędny zapis %s' % linein)
            iflags |= UDIF_ABBR
        else:
            for a in flags:
                if a in '1234':
                    if iflags & 0x70:
                        error('Błędny zapis %s' % linein)
                    iflags |= int(a) <<4
                    if not result:
                        iflags |= UDIF_COPY
                elif a == 'e':
                    if iflags & UDIF_ATEND:
                        error('Błędny zapis %s' % linein)
                    iflags |= UDIF_ATEND
                else:
                    error('Błędny zapis %s' % linein)
    if not result and not (iflags & (UDIF_COPY | UDIF_ABBR)):
        error('Błędny zapis %s' % linein)
    if result and not proper_str(result, True):
        error('Błędny zapis %s' % linein)
    pattern = asort(pattern)
    lines.append(bytes([iflags]) + repstr(pattern) + b'\0' + repstr(result))

f=open(sys.argv[1])
for line in f:
    line = line.split('//')[0].strip()
    if not line:
        continue
    if line.startswith('#'):
        addhash(line[1:].strip())
        continue
    add_udic(line)
    
def genstri(su):
    sx=[]
    for a in su:
        if a and a < 127 and a != ord("'") and a != ord('"') and a != ord('\\'):
            sx.append(chr(a))
        else:
            sx.append('\\%03o' % a)
    return '"'+''.join(sx)+'"'
def genstr(sb):
    ss = list(genstri(s) for s in sb)
    ss.append('NULL')
    return ',\n'.join(ss)

if defines:
    sout=['#ifndef USER_UNITS',
        '#define USER_UNITS %s' % (('%s_units' % uname) if units else 'NULL'),
        '#endif',
        '#ifndef USER_LINES',
        '#define USER_LINES %s' % (('%s_lines' % uname) if lines else 'NULL'),
        '#endif'
        ]
    if units:
        sout.append('static const char *const %s_units[]={%s};' % (uname, genstr(units)))
    if lines:
        sout.append('static const char *const %s_lines[]={%s};' % (uname, genstr(lines)))
else:
    sout = [
        'static const char *const %s_units[]={%s};' % (uname, genstr(units)),
        'static const char *const %s_lines[]={%s};' % (uname, genstr(lines))]
    
sout = '\n'.join(sout)
print(sout)
if outname:
    open(outname,'w').write(sout+'\n')
