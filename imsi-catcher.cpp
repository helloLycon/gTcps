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
#include <ctype.h>
#include <vector>
#include "imsi-catcher.h"
#include "tcpServer.h"
#include "cliRoutine.h"

using namespace std;

pthread_mutex_t imsiMutex = PTHREAD_MUTEX_INITIALIZER;
char imsi_to_catch[16] = "000000000000000";

#if 0
const CommandInfo command_info[] = {
    [SET_IMSI_BLACKLIST] = {
        .frame_proto = "32",
        .name = "SET_IMSI_BLACKLIST",
    },
    [SET_PROBE_EARFCN] = {
        .frame_proto = "31",
        .name = "SET_PROBE_EARFCN",
    },
    [SET_PCI] = {
        .frame_proto = "02",
        .name = "SET_PCI",
    },
    [REBOOT_CELL] = {
        .frame_proto = "20",
        .name = "REBOOT_CELL",
    },
    [NET_INFO] = {
        .frame_proto = "01",
        .name = "NET_INFO",
    },
    [SIB5] = {
        .frame_proto = "05",
        .name = "SIB5",
    },
};
#else
CommandInfo command_info[COMMAND_TYPE_COUNT] = {0};

void command_info_init(CommandInfo *info) {
    info[SET_IMSI_BLACKLIST].frame_proto = "32";
    info[SET_IMSI_BLACKLIST].name = "SET_IMSI_BLACKLIST";
    info[SET_PROBE_EARFCN].frame_proto = "31";
    info[SET_PROBE_EARFCN].name = "SET_PROBE_EARFCN";
    info[SET_PCI].frame_proto = "02";
    info[SET_PCI].name = "SET_PCI";
    info[REBOOT_CELL].frame_proto = "20";
    info[REBOOT_CELL].name = "REBOOT_CELL";
    info[NET_INFO].frame_proto = "01";
    info[NET_INFO].name = "NET_INFO";
    info[SIB5].frame_proto = "05";
    info[SIB5].name = "SIB5";
}
#endif

vector<EarfcnInfo> earfcn_vect;

bool is_imsi(const char *str) {
    int len = strlen(str);
    if(len != 15) {
        return false;
    }
    for(int i=0; i<len; i++) {
        if(!isdigit(str[i])) {
            return false;
        }
    }
    if(strncmp(str, "460", 3)) {
        return false;
    }
    return true;
}

ssize_t new_read(int fd, void *buf, size_t count) {
    return read(curr_fd(), buf, count);
}

ssize_t new_write(int fd, const void *buf, size_t count) {
    return write(curr_fd(), buf, count);
}


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

CommandType get_cmd_type_from_frame(void *frame) {
    BaseFrame *base = (BaseFrame *)frame;
    int i;
    for(i=0; i<sizeof(command_info)/sizeof(command_info[0]); i++) {
        if(!strncmp(command_info[i].frame_proto, base->frame_proto, 2)) {
            break;
        }
    }
    return (CommandType)i;
}


void dump_frame(bool send, void *frame, int len) {
    if(is_valid_frame(frame, len)) {
        BaseFrame *base = (BaseFrame *)frame;
        char frame_type[4] = {0}, frame_proto[4] = {0};
        char frame_body[10240] = {0};
        memcpy(frame_type, base->frame_type, 2);
        memcpy(frame_proto, base->frame_proto, 2);
        memcpy(frame_body, base->data, ntohl(base->data_size) - 4);
        printf("[%s%s, %s-%s] %s\n", send?"-> ":"<- ", get_cmd_type_from_frame(frame)==COMMAND_TYPE_COUNT?"unknown":command_info[get_cmd_type_from_frame(frame)].name, frame_type, frame_proto, frame_body);
    } else {
        fprintf(stderr, "%s: invalid format\n", __func__);
    }
}

unsigned data_size(const char *data) {
    return htonl(strlen(data) + 4);
}

#define PROBE_EARFCN_LIST  "100,300,525,1300,1506,1650,1825,1850,38950,39148,39250,40340"
//#define PROBE_EARFCN_LIST  "100"

int parse_int_following_keyword(const char *haystack, const char *kw) {
    const char *find = strstr(haystack, kw);
    if(NULL == find) {
        return -999999;
    } else {
        return atoi( find + strlen(kw));
    }
}

int plmn_to_operator(int plmn) {
    switch(plmn) {
        case 46000:
        case 46002:
        case 46004:
        case 46007:
            /* 移动 */
            return 0;
        case 46001:
        case 46006:
        case 46009:
            /* 联通 */
            return 1;
        case 46003:
        case 46005:
        case 46011:
            /* 电信 */
            return 2;
        default:
            return 100;
    }
}

int earfcn_to_band(int earfcn) {
    struct BandEarfcnMap {
        int band;
        int lower;
        int upper;
    };
    static const BandEarfcnMap band_map[] = {
        {
            /*.band =*/ 1,
            /*.lower =*/ 0,
            /*.upper =*/ 599,
        },
        {
            /*.band =*/ 3,
            /*.lower =*/ 1200,
            /*.upper =*/ 1949,
        },
        {
            /*.band =*/ 5,
            /*.lower =*/ 2400,
            /*.upper =*/ 2649,
        },
        {
            /*.band =*/ 8,
            /*.lower =*/ 3450,
            /*.upper =*/ 3799,
        },
        {
            /*.band =*/ 34,
            /*.lower =*/ 36200,
            /*.upper =*/ 36349,
        },
        {
            /*.band =*/ 38,
            /*.lower =*/ 37750,
            /*.upper =*/ 38249,
        },
        {
            /*.band =*/ 39,
            /*.lower =*/ 38250,
            /*.upper =*/ 38649,
        },
        {
            /*.band =*/ 40,
            /*.lower =*/ 38650,
            /*.upper =*/ 39649,
        },
        {
            /*.band =*/ 41,
            /*.lower =*/ 39650,
            /*.upper =*/ 41589,
        },
    };
    for(int i = 0; i<sizeof(band_map)/sizeof(band_map[0]); i++) {
        if(earfcn>= band_map[i].lower && earfcn <= band_map[i].upper) {
            return band_map[i].band;
        }
    }
    return 0;
}

void handle_earfcn_list_data(const char *str) {
    const char *earfcn_keyword = "EARFCN:";
    const char *pci_keyword = "PCI:";
    const char *rsrp_keyword = "RSRP:";
    const char *tac_keyword = "TAC:";
    const char *plmn_keyword = "PLMNID:";
    int cnter = 0;
    for(char *line_start = (char *)str; ; cnter++) {
        char line[1024] = {0};
        char *lr = strstr(line_start, "\r\n");
        if(NULL == lr) {
            return;
        }
        memcpy(line, line_start, lr + 2 -line_start);
        line_start = lr +2;

        /* handle data */
        //printf("(%d) %s", cnter, line);
        EarfcnInfo item;
        item.earfcn = parse_int_following_keyword(line, earfcn_keyword);
        item.pci = parse_int_following_keyword(line, pci_keyword);
        item.plmn = parse_int_following_keyword(line, plmn_keyword);
        item.oper = plmn_to_operator(item.plmn);
        item.band = earfcn_to_band(item.earfcn);
        earfcn_vect.push_back(item);
    }
}

void handle_earfcn_pri_data(const char *str) {
    const char *earfcn_keyword = "EARFCN:";
    const char *pri_keyword = "PRI:";
    int cnter = 0;
    for(char *line_start = (char *)str; ; cnter++) {
        char line[1024] = {0};
        char *lr = strstr(line_start, "\r\n");
        if(NULL == lr) {
            return;
        }
        memcpy(line, line_start, lr + 2 -line_start);
        line_start = lr +2;

        /* handle data */
        //printf("(%d) %s", cnter, line);
        int earfcn = parse_int_following_keyword(line, earfcn_keyword);
        int pri = parse_int_following_keyword(line, pri_keyword);
        for(vector<EarfcnInfo>::iterator it = earfcn_vect.begin(); it != earfcn_vect.end(); it++  ) {
            if(it->earfcn == earfcn) {
                it->pri = pri;
            }
        }
    }
}

bool lte_probe(int fd) {
    /* 检查公网扫描信息是否已经存在了 */
    if(earfcn_vect.empty() == false) {
        /*--- dump vector ---*/
        printf("<--------- earfcn list --------->\n");
        for(vector<EarfcnInfo>::iterator it = earfcn_vect.begin(); it != earfcn_vect.end(); it++  ) {
            printf("earfcn:%d\tpri:%d\tpci:%d\tplmn:%d\toper:%d\tband:%d\n", it->earfcn, it->pri, it->pci, it->plmn, it->oper, it->band);
        }
        return true;
    }

    //static bool sent = false;
    char buf[10240] = {0};
    BaseFrame *base = (BaseFrame *)buf;

    if(/*sent == false*/ true) {
        fill_frame_hdr(buf, "01", "01");
        strcpy(base->data, "LTEREM:1\tEARFCN:" PROBE_EARFCN_LIST "\tGSMREM:0\tREMPRD:0\tAUTOCFG:0\r\n");
        base->data_size = data_size(base->data);
        int send_len = sizeof(BaseFrame) + strlen(base->data);
        dump_frame(true, buf, send_len);
        //hex("send", buf, send_len);
        while(send_len != new_write(fd, buf, send_len)) {
            perror("new_write(socket)");
            sleep(2);
        }
        //sent = true;
    }

    /* 开始获取公网扫描信息 */
    bool require_earfcn_pri = false;
    for(time_t start_time = time(NULL); time(NULL) - start_time < 5*60;) {
        int ret = new_read(fd, buf, sizeof buf);
        if(ret < 0) {  /* disconnect, etc. */
            perror("new_read(socket)");
            sleep(2);
            continue;
        } else if(0 == ret) {
            /* do nothing */
        } else {
            if(false == is_valid_frame(buf, ret))  {
                fprintf(stderr, "invalid frame\n");
                continue;
            }
            dump_frame(false, buf, ret);
            if(require_earfcn_pri == false) {
                if(check_frame_type_proto(buf, "02", "01")) {
                    int remain_len = ntohl(base->data_size)-4+sizeof(BaseFrame) - ret;
                    for(int current_len = ret; remain_len > 0; ) {
                        /* 准备接受分片 */
                        int append_len = new_read(fd, buf + current_len, sizeof(buf) - current_len);
                        if( append_len < 0 ) {
                            perror("new_read(socket)");
                            sleep(2);
                            continue;
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
                        int append_len = new_read(fd, buf + current_len, sizeof(buf) - current_len);
                        if( append_len < 0 ) {
                            perror("new_read(socket)");
                            sleep(2);
                            continue;
                        }
                        remain_len -= append_len;
                        current_len += append_len;
                    }
                    /* 03-05 success */
                    buf[sizeof(BaseFrame) + ntohl(base->data_size)-4] = '\0';
                    printf("[03-05 pri] %s\n", base->data);
                    handle_earfcn_pri_data(base->data);

                    /*--- dump vector ---*/
                    printf("<--------- earfcn list --------->\n");
                    for(vector<EarfcnInfo>::iterator it = earfcn_vect.begin(); it != earfcn_vect.end(); it++  ) {
                        printf("earfcn:%d\tpri:%d\tpci:%d\tplmn:%d\toper:%d\tband:%d\n", it->earfcn, it->pri, it->pci, it->plmn, it->oper, it->band);
                    }
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
    /* check validation */
    if(false == is_valid_frame(buf, len)) {
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
    while(send_len != new_write(fd, buf, send_len)) {
        perror("new_write(socket)");
        sleep(2);
    }

    for(int i=0; i<10; i++) {
        int ret = new_read(fd, buf, sizeof buf);
        if(ret < 0) {
            perror("new_read(socket)");
            sleep(2);
            continue;
        }
        bool result;
        if(general_check_response(buf, ret, cmd, &result)) {
            printf("command(%s) result = %d %s\n", command_info[cmd].name, result, result?"success":"fail");
            return result;
        }
    }
    fprintf(stderr, "%s: timeout\n", __func__);
    return false;
}

bool set_imsi_blacklist(int fd, const char *imsi) {
    char data[1024];
    sprintf(data, "MODE:3\tIMSI:%s\r\n", imsi);
    return execute_command(fd, SET_IMSI_BLACKLIST, data);
}

bool set_probe_earfcn(int fd, const int *earfcn_list) {
    char data[1024];
    int offset = sprintf(data, "POLLFCN:");
    for(int i=0; earfcn_list[i]; i++) {
        offset += sprintf(data + offset, "%d,", earfcn_list[i]);
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

bool reboot_cell(int fd) {
    return execute_command(fd, REBOOT_CELL, "");
}

int imsi_catcher_routine(int fd, const char *imsi) {
    while(false == lte_probe(fd)) {
        fprintf(stderr, "NET_INFO failed, retry\n");
        sleep (5);
    }

    /* calculate earfcns to probe */
    char plmn_str[64];
    strcpy(plmn_str, imsi);
    plmn_str[5] = '\0';
    int plmn = atoi(plmn_str);
    int oper = plmn_to_operator(plmn);
    printf("=> operator = %d\n", oper);

    /* list bands in supposed operator */
    int band_list[16] = {0};
    for(vector<EarfcnInfo>::iterator it = earfcn_vect.begin(); it!=earfcn_vect.end(); it++) {
        if(it->oper != oper) {
            continue;
        }
        int index;
        for(index=0; index<sizeof(band_list)/sizeof(band_list[0]) && band_list[index]; index++) {
            if(it->band == band_list[index]) {
                break;
            }
        }
        band_list[index] = it->band;
    }
    /* find top priority earfcn in each band */
    int earfcns[16] = {0};
    for(int i=0; band_list[i]; i++) {
#if  0
        EarfcnInfo min_pri = {
            .pri = 999,
        };
#else
        EarfcnInfo min_pri = {0};
        min_pri.pri = 999;
#endif
        for(vector<EarfcnInfo>::iterator it = earfcn_vect.begin(); it!=earfcn_vect.end(); it++) {
            if(it->oper == oper && it->band == band_list[i] && it->pri < min_pri.pri) {
                min_pri = *it;
            }
        }
        earfcns[i] = min_pri.earfcn;
        printf("Band %d: %d(pri=%d)\n", band_list[i], earfcns[i], min_pri.pri);
    }

    
    if(set_imsi_blacklist(fd, imsi) == false) {
        fprintf(stderr, "set_imsi_blacklist failed\n");
        return -1;
    }
    if(set_probe_earfcn(fd, earfcns) == false) {
        fprintf(stderr, "set_probe_earfcn failed\n");
        return -1;
    }

    /* mod3 calculation */
    int mod_value[3] = {0};
    for(vector<EarfcnInfo>::iterator it = earfcn_vect.begin(); it!=earfcn_vect.end(); it++) {
        mod_value[it->pci % 3]++;
    }
    int min_count_index = mod_value[0] < mod_value[1]?0:1;
    min_count_index = mod_value[min_count_index]<mod_value[2]?min_count_index:2;
    printf("=> min_count_index = %d\n", min_count_index);
    if(set_pci(fd, 501 + min_count_index) == false) {
        fprintf(stderr, "set_pci failed\n");
        return -1;
    }
    if(reboot_cell(fd) == false) {
        fprintf(stderr, "reboot_cell failed\n");
        return -1;
    }
    pthread_mutex_lock(&imsiMutex);
    strcpy(imsi_to_catch, "000000000000000");
    pthread_mutex_unlock(&imsiMutex);
    printf("=> imsi command success\n");
    return true;
}



void *imsi_catcher_thread(void *arg) {
    command_info_init(command_info);
    for(;;) {
        char imsi[32];
        pthread_mutex_lock(&imsiMutex);
        strcpy(imsi, imsi_to_catch);
        pthread_mutex_unlock(&imsiMutex);
        if(is_imsi(imsi)) {
            imsi_catcher_routine(0, imsi);
        } else {
            char buf[1024];
            int ret = new_read(0, buf, sizeof buf);
            if(ret < 0) {
                perror("new_read(socket)");
                sleep(2);
                continue;
            } else if(is_valid_frame(buf, ret)) {
                dump_frame(false, buf, ret);
            }
        }
    }
}

