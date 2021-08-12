#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include "imsi-catcher.h"


const CommandInfo command_info[] = {
    [ADD_BLACKLIST_IMSI] = {
        .frame_proto = "10",
    },
    [SET_PCI] = {
        .frame_proto = "02",
    },
    [SET_PROBE_EARFCN] = {
        .frame_proto = "31",
    },
};

void hex(const char *hdr, const void *frame, int n) {
    printf("[%s] ", hdr);
    for(int i=0; i<n; i++) {
        printf("%02x ", ((const unsigned char *)frame)[i] );
    }
    printf("\n");
}

void fill_frame_hdr(void *buf, const char *frame_type, const char *frame_proto) {
    BaseFrame *base = (BaseFrame *)buf;
    base->magic = FRAME_MAGIC;
    memcpy(base->frame_type, frame_type, 2);
    memcpy(base->frame_proto, frame_proto, 2);
}

bool check_frame_type_proto(void *buf, const char *type, const char *proto) {
    BaseFrame *base = (BaseFrame *)buf;
    return base->magic == FRAME_MAGIC && (!strncmp(base->frame_type, type, 2)) && (!strncmp(base->frame_proto, proto, 2));
}


bool is_valid_frame(void *frame, int len) {
    BaseFrame *base = (BaseFrame *)frame;
    /* check magic number */
    if(base->magic != FRAME_MAGIC) {
        return false;
    }
    /* check length */
    if(len <= sizeof(BaseFrame)) {
        return false;
    }
    /* 公网扫描上报，不检查长度，可能是分片的 */
    if(check_frame_type_proto(frame, "02", "01") || check_frame_type_proto(frame, "03", "05")) {
        return true;
    }
    if (ntohl(base->data_size) != len - sizeof(BaseFrame) + 4) {
        return false;
    }
    return true;
}

void dump_frame(int fd, void *frame, int len) {
    if(is_valid_frame(frame, len)) {
        BaseFrame *base = (BaseFrame *)frame;
        char frame_type[4] = {0}, frame_proto[4] = {0};
        char frame_body[10240] = {0};
        memcpy(frame_type, base->frame_type, 2);
        memcpy(frame_proto, base->frame_proto, 2);
        memcpy(frame_body, base->data, ntohl(base->data_size) - 4);
        printf("[fd=%d, %s-%s] %s\n", fd, frame_type, frame_proto, frame_body);
    } else {
        fprintf(stderr, "%s: invalid format\n", __func__);
    }
}

unsigned data_size(const char *data) {
    return htonl(strlen(data) + 4);
}

//#define PROBE_EARFCN_LIST  "100,300,525,1300,1506,1650,1825,1850,38950,39148,39250,40340"
#define PROBE_EARFCN_LIST  "100,300"

void handle_earfcn_list_data(const char *str) {
    int cnter = 0;
    for(char *line_start = (char *)str; ; cnter++) {
        char line[1024] = {0};
        char *lr = strstr(line_start, "\r\n");
        if(NULL == lr) {
            return;
        }
        memcpy(line, line_start, lr + 2 -line_start);
        printf("(%d) %s", cnter, line);
        line_start = lr +2;
    }
}

void handle_earfcn_pri_data(const char *str) {
    int cnter = 0;
    for(char *line_start = (char *)str; ; cnter++) {
        char line[1024] = {0};
        char *lr = strstr(line_start, "\r\n");
        if(NULL == lr) {
            return;
        }
        memcpy(line, line_start, lr + 2 -line_start);
        printf("(%d) %s", cnter, line);
        line_start = lr +2;
    }
}

bool lte_probe(int fd) {
    static bool sent = false;
    char buf[10240] = {0};
    BaseFrame *base = (BaseFrame *)buf;

    if(sent == false) {
        fill_frame_hdr(buf, "01", "01");
        strcpy(base->data, "LTEREM:1\tEARFCN:" PROBE_EARFCN_LIST "\tGSMREM:0\tREMPRD:0\tAUTOCFG:0\r\n");
        base->data_size = data_size(base->data);
        int send_len = sizeof(BaseFrame) + strlen(base->data);
        dump_frame(fd, buf, send_len);
        hex("send", buf, send_len);
        if(send_len != write(fd, buf, send_len)) {
            perror("write(socket)");
            return -1;
        }
        sent = true;
    }

    /* 开始获取公网扫描信息 */
    bool require_earfcn_pri = false;
    for(time_t start_time = time(NULL); time(NULL) - start_time < 5*60;) {
        int ret = read(fd, buf, sizeof buf);
        if(ret < 0) {  /* disconnect, etc. */
            perror("read(socket)");
            return false;
        } else if(0 == ret) {
            /* do nothing */
        } else {
            if(false == is_valid_frame(buf, ret))  {
                fprintf(stderr, "invalid frame\n");
                continue;
            }
            dump_frame(fd, buf, ret);
            if(require_earfcn_pri == false) {
                if(check_frame_type_proto(buf, "02", "01")) {
                    int remain_len = ntohl(base->data_size)-4+sizeof(BaseFrame) - ret;
                    for(int current_len = ret; remain_len > 0; ) {
                        /* 准备接受分片 */
                        int append_len = read(fd, buf + current_len, sizeof(buf) - current_len);
                        if( append_len < 0 ) {
                            perror("read(socket)");
                            return false;
                        }
                        remain_len -= append_len;
                        current_len += append_len;
                    }
                    /* 02-01 success */
                    buf[sizeof(BaseFrame) + ntohl(base->data_size)-4] = '\0';
                    printf("[02-01 earfcn] %s\n", base->data);
                    handle_earfcn_list_data(base->data);
                    require_earfcn_pri = true;
                }
            } else {
                if(check_frame_type_proto(buf, "03", "05")) {
                    int remain_len = ntohl(base->data_size)-4+sizeof(BaseFrame) - ret;
                    for(int current_len = ret; remain_len > 0; ) {
                        /* 准备接受分片 */
                        int append_len = read(fd, buf + current_len, sizeof(buf) - current_len);
                        if( append_len < 0 ) {
                            perror("read(socket)");
                            return false;
                        }
                        remain_len -= append_len;
                        current_len += append_len;
                    }
                    /* 03-05 success */
                    buf[sizeof(BaseFrame) + ntohl(base->data_size)-4] = '\0';
                    printf("[03-05 pri] %s\n", base->data);
                    handle_earfcn_pri_data(base->data);
                    return true;
                }
            }
        }
    }
    fprintf(stderr, "%s timeout\n", __func__);
    return false;
}

bool general_check_response(void *buf, int len, CommandType cmd, bool *result) {
    BaseFrame *base = (BaseFrame *)buf;
    /* 长度过短 */
    if(len <= sizeof(BaseFrame)) {
        return false;
    }
    /* 长度不正确 */
    if(ntohl(base->data_size) + sizeof(BaseFrame) -4 != len) {
        fprintf(stderr, "wrong data_size field\n");
        return false;
    }
    if(check_frame_type_proto(buf, "02", command_info[cmd].frame_proto)) {
        if(base->data[0] == (unsigned char)'0') {
            *result = true;
        } else {
            *result = false;
        }
        return true;
    } else {
        return false;
    }
}

bool execute_command(int fd, CommandType cmd, const char *frame_body) {
    char buf[1024] = {0};
    BaseFrame *base = (BaseFrame *)buf;
    fill_frame_hdr(buf, "01", command_info[cmd].frame_proto);

    char *data = base->data;
    strcpy(data, frame_body);

    base->data_size = data_size(data);
    int send_len = sizeof(BaseFrame) + strlen(data);
    if(send_len != write(fd, buf, send_len)) {
        perror("write(socket)");
        return false;
    }

    for(int i=0; i<5; i++) {
        int ret = read(fd, buf, sizeof buf);
        if(ret < 0) {
            perror("read(socket)");
            return false;
        }
        bool result;
        if(general_check_response(buf, ret, cmd, &result)) {
            printf("result = %d\n", result);
            return result;
        }
    }
    fprintf(stderr, "%s: timeout\n", __func__);
    return false;
}

bool add_imsi_into_blacklist(int fd, const char *imsi) {
    char data[1024];
    sprintf(data, "BLACKLIST:%s\r\n", imsi);
    return execute_command(fd, ADD_BLACKLIST_IMSI, data);
}

bool set_probe_earfcn(int fd, const int *earfcn_list) {
    char data[1024];
    int offset = sprintf(data, "POLLFCN:");
    for(int i=0; earfcn_list[i]; i++) {
        offset += sprintf(data + offset, "%d\t", earfcn_list[i]);
    }
    offset--;
    offset += sprintf(data + offset, "\r\n");
    printf("%s data: %s\n", __func__, data);
    return execute_command(fd, SET_PROBE_EARFCN, data);
}

bool set_pci(int fd, int pci) {
    char data[1024];
    sprintf(data, "PCI:%d\r\n", pci);
    return execute_command(fd, SET_PCI, data);
}

int imsi_catcher_routine(int fd, const char *imsi) {
    lte_probe(fd);
    sleep(1000000);
}


