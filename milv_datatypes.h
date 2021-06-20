struct sfxdata {
    unsigned int offset:20;
    unsigned int stress:3;
    unsigned int flags:2;
    };

struct locvdic {
    unsigned int offset:20;
    unsigned int stress:3;
    unsigned int typ:3;
    unsigned int flags:6;
};

struct vobsuffix {
    unsigned int suffix_offset:20;
    unsigned int vob_len:12;
};

struct udict {
    unsigned int pattern_offset:20;
    unsigned int value_offset:20;
    unsigned int stress:3;
    unsigned int flags:8;
};

struct rcgrecognize {
    unsigned int offset:20;
    unsigned int sayas_pos: 9;
    unsigned int flags:3;
};

struct rcgunit {
    unsigned int name_offset:20;
    unsigned int value_offset:20;
    unsigned int fem:1;
};
    
    
struct choicercg {
    uint16_t table_offset;
    uint16_t table_len;
};

struct choicetable {
    uint32_t pattern_offset;
    uint32_t value_offset;
};

struct binlet_ruleset {
    unsigned int deftrans:24;
    unsigned int letter:8;
    uint16_t first_rule;
    uint16_t rule_count;
};

struct binlet_rule {
    uint16_t left;
    uint16_t right;
    int trans:20;
    int eat:12;
};
