#ifndef __IMSI_CATCHER_H
#define __IMSI_CATCHER_H


#define FRAME_MAGIC  0x00FFFF00



struct BaseFrame{
    unsigned magic;
    unsigned id;
    unsigned data_size;
    unsigned encrypt_length;
    unsigned crc;
    char device_name[16];
    unsigned timestamp;
    char reserve[8];
    char frame_type[2];
    char frame_proto[2];
    char data[0];
} __attribute__((packed)) ;


enum CommandType {
    SET_IMSI_BLACKLIST = 0,
    SET_PROBE_EARFCN,
    SET_PCI,
    REBOOT_CELL,
    NET_INFO,
    SIB5,
};

struct CommandInfo {
    //CommandType type;
    //const char *frame_type;
    const char *frame_proto;
    const char *name;
} ;


struct EarfcnInfo{
    int earfcn;
    int pri;
    int pci;
    int plmn;
    int oper;
    int band;
} ;


int imsi_catcher_routine(int fd, const char *imsi);



#endif

