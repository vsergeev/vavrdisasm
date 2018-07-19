#ifndef AVR_CHIPINFO_SET_H
#define AVR_CHIPINFO_SET_H

struct AvrChipIoRegInfo {
    unsigned int address;
    char name[10];
    char *comment;
};

struct AvrChipIntInfo {
    unsigned int address;
    char *name;
    char *comment;
};

struct AvrChipInfo {
    char name[10];

    unsigned int interruptCount;
    struct AvrChipIntInfo *interrupts;

    unsigned int ioRegCount;
    struct AvrChipIoRegInfo *ioRegs;
};

extern struct AvrChipInfo AVR_ChipInfo_Set[];
extern int AVR_TOTAL_CHIPINFOS;

#endif
