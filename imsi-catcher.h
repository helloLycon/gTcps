#ifndef __IMSI_CATCHER_H
#define __IMSI_CATCHER_H


#define FRAME_MAGIC  0x00FFFF00



typedef struct {
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
} __attribute__((packed)) BaseFrame;


enum CommandType {
    ADD_BLACKLIST_IMSI,
    SET_PCI,
    SET_PROBE_EARFCN,
};

typedef struct {
    //CommandType type;
    //const char *frame_type;
    const char *frame_proto;
} CommandInfo;


int imsi_catcher_routine(int fd, const char *imsi);



#endif

