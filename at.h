/************************************************
    File name: at.h
    Author: 柳文剑
    Version:0.0.1
    Date:2018-4-19
    Description:主要功能是向串口发送AT指令
    Others:
*************************************************/
#ifndef __AT_H__
#define __AT_H__

#define IMSI_LEN            15                              /* imsi  长度 */
#define ICCID_LEN           20                              /* iccid 长度 */
#define MAX_SEND_CMD_TIME   40                              /* at 指令最大发送次数 */

#define WRITE_FILE 0
#define NOT_WRITE_FILE 1

#define SET_GET_BASE_STA    "CREG=2"                        /* 设置获取基站信息标志 */
#define GET_BASE_STA        "CREG"                          /* 获取基站信息标志 */
#define IMSI_CMD            "CIMI"                          /* 获取imsi 指令标志 */
#define ICCID_CMD           "QCCID"                         /* 获取iccid 指令标志 */
#define SIM_STATUS_CMD      "CPIN"                          /* 获取sim 卡状态指令标志 */
#define SIGNAL_STRENGTH_CMD "CSQ"                           /* 获取信号强度指令标志 */
#define OPEN_GPS_CMD        "QGPS"                          /* 开启gps 指令标志 */
#define NETWORK_CMD         "QNWINFO"                       /* 获取当前网络标志 */
#define RET_ERROR           "ERROR"                         /* 返回错误 */
#define NETNONE             "NONE"                          /* 无网络 */
#define TELECOM_2G          "CDMA1X"                        /* 电信2G */
#define TELECOM_2_3G_1      "CDMA1X AND HDR"                /* 电信2/3G */
#define TELECOM_2_3G_2      "CDMA1X AND EHRPD"              /* 电信2/3G */
#define TELECOM_3G_1        "HDR"                           /* 电信3G */
#define TELECOM_3G_2        "HDR-EHRPD"                     /* 电信3G */
#define MOBILE_UNICOM_2G_1  "GSM"                           /* 移动或者联通2G */
#define MOBILE_UNICOM_2G_2  "GPRS"                          /* 移动或者联通2G  */
#define MOBILE_UNICOM_2G_3  "EDGE"                          /* 移动或者联通2G  */
#define UNICOM_3G_1         "WCDMA"                         /* 联通3G */
#define UNICOM_3G_2         "HSDPA"                         /* 联通3G */
#define UNICOM_3G_3         "HSUPA"                         /* 联通3G */
#define UNICOM_3G_4         "HSPA+"                         /* 联通3G */
#define MOBILE_3G           "TDSCDMA"                       /* 移动3G */
#define MOBILE_4G           "TDD LTE"                       /* 移动4G */
#define TELECOM_UNICOM_4G   "FDD LTE"                       /*  电信或联通4G */
#define RET_OK              "OK"                            /* 返回成功标志 */

#define CUR_VERSION         "0.0.1"                         /* 当前版本号 */
#define IMSI_TMP_FILE       "/tmp/data/imsi.txt"            /* 获取imsi 临时文件 */
#define ICCID_TMP_FILE      "/tmp/data/iccid.txt"           /* 获取iccid 临时文件 */
#define CPIN_TMP_FILE       "/tmp/data/cpin.txt"            /* 获取sim 卡状态临时文件 */
#define CSQ_TMP_FILE        "/tmp/data/csq.txt"             /* 获取信号强度临时文件 */
#define OPEN_GPS_TMP_FILE   "/tmp/data/open_gps.txt"        /* 获取信号强度临时文件 */
#define NETWORK_FILE        "/tmp/data/network_mode.txt"    /* 获取信号网络模式临时文件 */
#define SET_BASE_STA_FILE   "/tmp/data/set_base_sta.txt"    /* 设置获取基站信息结果临时文件 */
#define BASE_STA_INFO_FILE  "/tmp/data/base_sta_info.txt"   /* 设置获取基站信息结果临时文件 */
#define VERSION_FILE        "/tmp/data/at_version"          /* 存储版本号的文件名 */

typedef struct serial_port_info
{
    char file[256];          /* 文件名 */
    int file_len;  /* 文件长度 */
    FILE *file_fp;

    const char *dev_name;         /* 串口名 */
    int baud_rate;          /* 波特率 */
    int uart_fd;
}serial_port_info;

/*****************************************************************
    Function: UART_Open
    Description: 打开一个串口
    Input: 
        port : 传入被打开的串口名
    Output: 
        fd : 获取描述符
    Return: 
        AT_ERROR_OPEN_FAILED    open 函数操作失败
        AT_ERROR_FCNTL_FAILED   fcntl 函数操作失败
        AT_OK                               成功
    Other:
*****************************************************************/
extern int UART_Open(char *port, int *fd);


/*****************************************************************
    Function: UART_Close
    Description: 关闭打开的串口
    Input: 
        fd : 串口描述符
    Output: 无
    Return:  无
    Other:
*****************************************************************/
extern void UART_Close(int fd);

/*****************************************************************
    Function: UART_Init
    Description: 设置打开的串口
    Input: 
        fd : 串口描述符
        speed : 波特率
        flow_ctrl : 数据流量控制
        databits : 数据位
        stopbits : 停止位
        parity : 校验位
    Output: 无
    Return: 
        AT_ERROR_TCGETATTR_FAILED   获取串口参数失败
        AT_ERROR_PARAMETER_ERROR   参数错误
        AT_ERROR_TCSETATTR_FAILED   设置串口参数失败
        AT_OK                                        成功
    Other:
*****************************************************************/
extern int UART_Init(int fd, int speed, int flow_ctrl, int databits, int stopbits, char parity);

/*****************************************************************
    Function: UART_Send
    Description:向串口发送数据
    Input: 
        fd : 串口描述符
        send_buf : 向串口发送的内容
        data_len : 向串口发送内容的长度
    Output: 无
    Return: 
        AT_ERROR_WRITE_FAILED    write 函数调用失败
        AT_OK                                 成功
    Other:
*****************************************************************/
extern int UART_Send(int fd, char *send_buf, int data_len);

/*****************************************************************
    Function: UART_Recv
    Description:读取串口数据
    Input: 
        fd : 串口描述符
        data_len : 读取最大长度
    Output: 
        rcv_buf : 从串口读取到的数据
        rcv_len : 从串口读取到数据的实际长度
    Return: 
        AT_ERROR_SELECT_FAILED  select 函数调用失败
        AT_OK                                 成功
    Other:
*****************************************************************/
extern int UART_Recv(int fd, int data_len, char *rcv_buf, int *rcv_len, int);

/*****************************************************************
    Function: at_cmd
    Description:执行at 指令
    Input: 
        dev_info : 串口相关参数信息
    Output: 
        ret_str : 串口返回结果
    Return: 
        AT_ERROR_OPEN_FAILED            open 函数操作失败
        AT_ERROR_FCNTL_FAILED           fcntl 函数操作失败
        AT_ERROR_TCGETATTR_FAILED   获取串口参数失败
        AT_ERROR_PARAMETER_ERROR   参数错误
        AT_ERROR_TCSETATTR_FAILED   设置串口参数失败
        AT_ERROR_SELECT_FAILED         select 函数调用失败
        AT_ERROR_WRITE_FAILED          write 函数调用失败
        AT_ERROR_RECV_FAILED            串口数据获取失败
        AT_OK                                       成功
    Other:
*****************************************************************/
extern int at_cmd(serial_port_info dev_info, char *ret_str);
int do_update_init(serial_port_info *dev_info);

#define  do_uart_init  do_update_init

extern serial_port_info uart_info;


#endif

