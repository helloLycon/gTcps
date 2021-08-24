/************************************************
    File name: at.c
    Author: 柳文剑
    Version:0.0.1
    Date:2018-4-19
    Description:主要功能是向串口发送AT指令
    Others:
*************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include "at.h"
#include "at_error.h"


void hexdump(const char *tip, const void *p, int len) {
    int i;
    printf("%s ", tip);
    for(i=0; i<len; i++) {
        printf("%02x ", ((const unsigned char *)p)[i] );
    }
    printf("\n");
}

/*****************************************************************
    Function: UART_Open
    Description: 打开一个串口
    Input: 
        port : 传入被打开的串口名
    Output: 
        fd : 获取描述符
    Return: 
        AT_ERROR_PARAMETER_ERROR  参数错误
        AT_ERROR_OPEN_FAILED    open 函数操作失败
        AT_ERROR_FCNTL_FAILED   fcntl 函数操作失败
        AT_OK                               成功
    Other:
*****************************************************************/
int UART_Open(const char *port, int *fd)
{
    int local_fd;

    /* 参数判断 */
    if ((NULL == port) || (NULL == fd))
    {
        return AT_ERROR_PARAMETER_ERROR;
    }

    /* 开启串口 */
    local_fd = open(port, O_RDWR|O_NOCTTY|O_NDELAY);
    if (-1 == local_fd)
    {
        perror("Can't Open Serial Port");
        return AT_ERROR_OPEN_FAILED;
    }
    
    //判断串口的状态是否为阻塞状态
    if (fcntl(local_fd, F_SETFL, 0) < 0)
    {
        printf("fcntl failed! errno = %s\n", strerror(errno));
        UART_Close(local_fd);
        return AT_ERROR_FCNTL_FAILED;
    } 

    *fd = local_fd;
    return AT_OK;
}

/*****************************************************************
    Function: UART_Close
    Description: 关闭打开的串口
    Input: 
        fd : 串口描述符
    Output: 无
    Return:  无
    Other:
*****************************************************************/
void UART_Close(int fd)
{
    int ret;
    
    ret = close(fd);

    if (ret < 0)
    {
        printf("file:%s, line:%d, ret = %d, fd = %d, errno = %d\r\n", 
            __FILE__, __LINE__, ret, fd, errno);
    }

    return ;
}

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
int UART_Init(int fd, int speed, int flow_ctrl, int databits, int stopbits, char parity)
{
    int i;
    int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300};
    int name_arr[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200, 300};
    struct termios options;

    /*
            tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options, 该函数,
            还可以测试配置是否正确，该串口是否可用等。若调用成功，函数返回值为0，
            若调用失败，函数返回值为1.
        */
    if (tcgetattr(fd, &options) != 0)
    {
        perror("SetupSerial 1");
        return AT_ERROR_TCGETATTR_FAILED;
    }

    /*设置串口输入波特率和输出波特率*/
    for (i = 0; i < (sizeof(speed_arr) / sizeof(int)); i++) 
    {     
        if (speed == name_arr[i]) 
        {
            cfsetispeed(&options, speed_arr[i]);
            cfsetospeed(&options, speed_arr[i]);
            break;
        }
    }

    if (i >= (sizeof(speed_arr) / sizeof(int)))
    {
        fprintf(stderr,"speed error\n");
        return AT_ERROR_PARAMETER_ERROR;
    }
    
    /* 修改控制模式，保证程序不会占用串口*/
    options.c_cflag |= CLOCAL;
    
    /* 修改控制模式，使得能够从串口中读取输入数据*/
    options.c_cflag |= CREAD;

    /*设置数据流控制*/
    switch (flow_ctrl)
    {
        case 0 : //不使用流控制
            options.c_cflag &= ~CRTSCTS;
            options.c_iflag &= ~(IXON | IXOFF | IXANY);
            break;
            
        case 1 : //使用硬件流控制
            options.c_cflag |= CRTSCTS;
            options.c_iflag &= ~(IXON | IXOFF | IXANY);
            break;
            
        case 2 : //使用软件流控制
            options.c_cflag &= ~(CRTSCTS);
            options.c_iflag |= IXON | IXOFF;
            break;
            
        default:
            fprintf(stderr,"flow_ctrl error\n");
            return AT_ERROR_PARAMETER_ERROR;
    }
    
    /*设置数据位*/
    options.c_cflag &= ~CSIZE; //屏蔽其他标志位
    switch (databits)
    {
        case 5 :
            options.c_cflag |= CS5;
            break;
            
        case 6 :
            options.c_cflag |= CS6;
            break;
            
        case 7 :
            options.c_cflag |= CS7;
            break;
            
        case 8 :
            options.c_cflag |= CS8;
            break;
            
        default :
            fprintf(stderr,"Unsupported data size\n");
            return AT_ERROR_PARAMETER_ERROR;
    }

    /* 设置校验位*/
    switch (parity) 
    {
        case 'n':
        case 'N': //无奇偶校验位。
            options.c_cflag &= ~(PARENB | PARODD);
            options.c_iflag &= ~INPCK;
            break;
            
        case 'o':
        case 'O': //设置为奇校验
            options.c_cflag |= (PARODD | PARENB);
            options.c_iflag |= INPCK;
            break;
            
        case 'e':
        case 'E': //设置为偶校验
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            options.c_iflag |= INPCK;
            break;
            
        case 's':
        case 'S': //设置为空格
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;
            
        default:
            fprintf(stderr,"Unsupported parity\n");
            return AT_ERROR_PARAMETER_ERROR;
    }

    /* 设置停止位*/
    switch (stopbits)
    {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
            
        case 2:
            options.c_cflag |= CSTOPB;
            break;
            
        default:
            fprintf(stderr,"Unsupported stop bits\n");
            return AT_ERROR_PARAMETER_ERROR;
    }
    
    /* 修改输出模式，原始数据输出*/
#if 0
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;
#else
    cfmakeraw(&options);
#endif

    /* 设置等待时间和最小接收字符*/
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */
    options.c_cc[VMIN] = 1; /* 读取字符的最少个数为1 */
    
    /* 如果发生数据溢出，接收数据，但是不再读取*/
    tcflush(fd, TCIOFLUSH);
    
    /*激活配置 (将修改后的termios数据设置到串口中）*/
    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        perror("com set error!/n");
        return AT_ERROR_TCSETATTR_FAILED;
    }
    
    return AT_OK;
}


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
        AT_ERROR_PARAMETER_ERROR 参数错误
        AT_ERROR_SELECT_FAILED  select 函数调用失败
        AT_ERROR_RECV_FAILED    接收失败
        AT_OK                                 成功
    Other:
*****************************************************************/
int UART_Recv(int fd, int data_len, char *rcv_buf, int *rcv_len, int timeout_ms)
{
    int len, fs_sel;
    fd_set fs_read;
    struct timeval time;

    if ((NULL == rcv_buf) || (NULL == rcv_len) || (data_len <= 0) || (fd < 0))
    {
        return AT_ERROR_PARAMETER_ERROR;
    }

    FD_ZERO(&fs_read);
    FD_SET(fd, &fs_read);

    time.tv_sec = timeout_ms/1000;
    time.tv_usec = (timeout_ms%1000)*1000;

    /* 使用select 实现串口的多路通信*/
    fs_sel = select((fd + 1), &fs_read, NULL, NULL, &time);
    if (fs_sel)
    {
        len = read(fd, rcv_buf, data_len);
        if (len < 0)
        {
            return AT_ERROR_RECV_FAILED;
        }
        *rcv_len = len;
        hexdump("<Rx> <-", rcv_buf, *rcv_len);
        return AT_OK;
    } 
    else 
    {
        return AT_ERROR_SELECT_FAILED;
    }    
}

/*****************************************************************
    Function: UART_Send
    Description:向串口发送数据
    Input: 
        fd : 串口描述符
        send_buf : 向串口发送的内容
        data_len : 向串口发送内容的长度
    Output: 无
    Return: 
        AT_ERROR_PARAMETER_ERROR 参数错误
        AT_ERROR_WRITE_FAILED    write 函数调用失败
        AT_OK                                 成功
    Other:
*****************************************************************/
int UART_Send(int fd, char *send_buf, int data_len)
{
    int ret;

    if ((NULL == send_buf) || (data_len <= 0))
    {
        return AT_ERROR_PARAMETER_ERROR;
    }

    hexdump("<Tx> =>", send_buf, data_len);
    ret = write(fd, send_buf, data_len);
    if (data_len == ret)
    {    
        return AT_OK;
    } 
    else 
    {
        if (-1 != ret)
        {
            tcflush(fd, TCOFLUSH);
        }
        return AT_ERROR_WRITE_FAILED;
    }    
}

unsigned char c_xor(const unsigned char *data, int len) {
    int i;
    unsigned char res = 0;
    for(i=0; i<len; i++) {
        res ^= data[i];
    }
    return res;
}

int do_update_init(serial_port_info *dev_info)
{
    int ret;
    int fd = -1;
    int rcv_len = 0;
    char rcv_buf[512];
    int i;
    int loop;
    char *tmp = NULL;

    /* 打开串口 */
    ret = UART_Open(dev_info->dev_name, &fd);
    if (AT_OK != ret)
    {
        fprintf(stderr, "UART_Open error\n");    
        return ret;
    }

    /* 初始化串口 */
    ret = UART_Init(fd, dev_info->baud_rate, 0, 8, 1, 'N');
    if (AT_OK != ret)
    {    
        fprintf(stderr, "Set Port Error\n");    
        UART_Close(fd);
        return ret;
    }
    dev_info->uart_fd = fd;
    dprintf(fd, "====================> uart initialized\n");
    return ret;
}

int flag_0x30_or_0xb0;
char file_buf[64*1024];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int flag_step1_finished = 0;


#ifdef __a
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
int at_cmd(serial_port_info dev_info, char *ret_str)
{
    int ret;
    int fd = -1;
    int rcv_len = 0;
    char rcv_buf[512];
    int i;
    int loop;
    char *tmp = NULL;

    if (NULL == ret_str)
    {
        return AT_ERROR_PARAMETER_ERROR;
    }

    /* 打开串口 */
    ret = UART_Open(dev_info.dev_name, &fd);
    if (AT_OK != ret)
    {
        printf("open error\n");    
        return ret;
    }

    /* 初始化串口 */
    ret = UART_Init(fd, dev_info.baud_rate, 0, 8, 1, 'N');
    if (AT_OK != ret)
    {    
        printf("Set Port Error\n");    
        UART_Close(fd);
        return ret;
    }

    /* 多次调用指令防止少量执行返回错误问题 */
    for (i = 0; i < MAX_SEND_CMD_TIME; i++)
    {
        /* 发送指令给串口 */
        ret = UART_Send(fd, dev_info.cmd, dev_info.cmd_len);
        if (AT_OK != ret)
        {
            UART_Close(fd);
            return ret;
        }

        /* 接收串口指令 */
        memset(rcv_buf, 0, sizeof(rcv_buf));
        ret = UART_Recv(fd, 512, rcv_buf, &rcv_len);
        
        if (AT_OK == ret)
        {
            if (NULL != strstr(dev_info.cmd, IMSI_CMD))
            {
                /* 获取imsi 先要判断是否有460 在里头,460 是国内sim 卡imsi 必有其实数值*/
                tmp = strstr(rcv_buf, "460");
                if (NULL != tmp)
                {
                    /* 确保imsi 长度足够长 */
                    if (strlen(tmp) >= IMSI_LEN)
                    {
                        rcv_buf[rcv_len]='\0'; 
                        memcpy(ret_str, tmp, IMSI_LEN);
                        UART_Close(fd);
                        return AT_OK;
                    }
                }
                else if (NULL != strstr(rcv_buf, RET_ERROR))
                {
                    UART_Close(fd);
                    return AT_ERROR_RECV_FAILED;
                }
            }
            else if (NULL != strstr(dev_info.cmd, ICCID_CMD))
            {
                /* 获取iccid 先要判断是否有8986 在里头,8986 是国内sim 卡iccid 必有其实数值*/
                tmp = strstr(rcv_buf, "8986");
                if (NULL != tmp)
                {
                    if (strlen(tmp) >= ICCID_LEN)
                    {
                        rcv_buf[rcv_len]='\0'; 
                        memcpy(ret_str, tmp, ICCID_LEN);
                        UART_Close(fd);
                        return AT_OK;
                    }
                }
                else if (NULL != strstr(rcv_buf, RET_ERROR))
                {
                    UART_Close(fd);
                    return AT_ERROR_RECV_FAILED;
                }
            }
            else if (NULL != strstr(dev_info.cmd, SIM_STATUS_CMD))
            {
                /* 获取sim 卡当前状态 */
                if (NULL != strstr(rcv_buf, "READY"))
                {
                    ret_str[0] = '0';
                    ret_str[1] = '\0';
                    UART_Close(fd);
                    return AT_OK;
                }
                else if (NULL != strstr(rcv_buf, RET_ERROR))
                {
                    if (NULL != strstr(rcv_buf, "10"))
                    {
                        /* 错误码10, 一般是未插sim 卡 */
                        ret_str[0] = '1';
                        ret_str[1] = '\0';
                    }
                    else if (NULL != strstr(rcv_buf, "13"))
                    {
                        /* 错误码13,一般是sim 卡被拔掉 */
                        ret_str[0] = '2';
                        ret_str[1] = '\0';
                    }
                    else
                    {
                        /* 其他错误码 */
                        ret_str[0] = '3';
                        ret_str[1] = '\0';
                    }
                    UART_Close(fd);
                    return AT_OK;
                }
            }
            else if (NULL != strstr(dev_info.cmd, SIGNAL_STRENGTH_CMD))
            {
                /* 获取信号强度 */
                tmp = strstr(rcv_buf, SIGNAL_STRENGTH_CMD);
                if (NULL != tmp)
                {
                    loop = 0;
                    while ((!isdigit(tmp[loop])) && (loop < rcv_len))
                    {
                        loop++;
                    }

                    /* 防止越界读取 */
                    if (loop < rcv_len)
                    {
                        strcpy(ret_str, tmp + loop);
                        strtok(ret_str, ",");
                        UART_Close(fd);
                        return AT_OK;
                    }
                }
            }
            else if (NULL != strstr(dev_info.cmd, OPEN_GPS_CMD))
            {
                /* gps 开启指令操作,OK 表示开启成功, 错误码504 表示已经开启成功重复开启 */
                if((NULL != strstr(rcv_buf, "ERROR: 504")) || (NULL != strstr(rcv_buf, "OK")))
                {
                    memcpy(ret_str, "OK", 2);
                    ret_str[2] = '\0';
                    UART_Close(fd);
                    return AT_OK;
                }
            }
            else if (NULL != strstr(dev_info.cmd, NETWORK_CMD))
            {
                /* 获取网络模式 */
                if ((NULL != strstr(rcv_buf, MOBILE_4G)) || (NULL != strstr(rcv_buf, TELECOM_UNICOM_4G)))
                {
                    ret_str[0] = '4';
                    ret_str[1] = '\0';
                    UART_Close(fd);
                    return AT_OK;
                }               
                else if ((NULL != strstr(rcv_buf, TELECOM_2_3G_1)) || (NULL != strstr(rcv_buf, TELECOM_2_3G_2)) || 
                    (NULL != strstr(rcv_buf, TELECOM_3G_1)) || (NULL != strstr(rcv_buf, TELECOM_3G_2)) || 
                    (NULL != strstr(rcv_buf, UNICOM_3G_1)) || (NULL != strstr(rcv_buf, UNICOM_3G_2)) || 
                    (NULL != strstr(rcv_buf, UNICOM_3G_3)) || (NULL != strstr(rcv_buf, UNICOM_3G_4)) || 
                    (NULL != strstr(rcv_buf, MOBILE_3G)))
                {
                    ret_str[0] = '3';
                    ret_str[1] = '\0';
                    UART_Close(fd);
                    return AT_OK;
                }
                else if ((NULL != strstr(rcv_buf, TELECOM_2G)) || (NULL != strstr(rcv_buf, MOBILE_UNICOM_2G_1)) || 
                    (NULL != strstr(rcv_buf, MOBILE_UNICOM_2G_2)) || (NULL != strstr(rcv_buf, MOBILE_UNICOM_2G_3)))
                {
                    ret_str[0] = '2';
                    ret_str[1] = '\0';
                    UART_Close(fd);
                    return AT_OK;
                }
                else if (NULL != strstr(rcv_buf, NETNONE))
                {
                    ret_str[0] = '0';
                    ret_str[1] = '\0';
                    UART_Close(fd);
                    return AT_OK;
                }
            }
            else if (NULL != strstr(dev_info.cmd, SET_GET_BASE_STA))
            {
                ret_str[0] = '1';
                ret_str[1] = '\0';
                if (NULL != strstr(rcv_buf, RET_OK))
                {
                    ret_str[0] = '0';
                    ret_str[1] = '\0';
                    UART_Close(fd);
                    return AT_OK;
                }
            }
            else if (NULL != strstr(dev_info.cmd, GET_BASE_STA))
            {
                if (NULL != strstr(rcv_buf, GET_BASE_STA))
                {
                    if (rcv_len > 9)
                    {
                        tmp = strchr(rcv_buf, '\n');
                        strcpy(rcv_buf, tmp + 1);
                        tmp = strchr(rcv_buf, '\n');
                        *(tmp + 1) = '\0';
                        rcv_len = strlen(rcv_buf);
                        memcpy(ret_str, rcv_buf, rcv_len);
                        ret_str[rcv_len] = '\0';
                        UART_Close(fd);
                        return AT_OK;
                    }
                }
            }
        } 
        else
        {
            UART_Close(fd);
            return ret;
        }

        /* 串口指令不间断执行获取不到正确结果 */
        usleep(100000);
    }
    
    UART_Close(fd);
    return AT_ERROR_RECV_FAILED;
}
#endif

/*****************************************************************
    Function: get_result
    Description:获取指令执行结果
    Input: 
        cmd : 执行的指令
        buf : 指令执行的结果
    Output: 无
    Return: 无
    Other:
*****************************************************************/
void get_result(char *cmd, char *buf)
{
    FILE *fd = NULL;
    char imsi[16];
    char iccid[32];
    char tmp[256];
    int i = 0;
    int flag = WRITE_FILE;
    
    if (NULL != strstr(cmd, IMSI_CMD))
    {
        /* 获取imsi */
        memcpy(imsi, buf, IMSI_LEN);
        imsi[IMSI_LEN] = '\0';
        
        for (i = 0; i < IMSI_LEN; i++)
        {
            if (!isdigit(imsi[i]))
            {
                flag = NOT_WRITE_FILE;
                break;
            }
        }
        
        if (WRITE_FILE == flag)
        {
            snprintf(tmp, 255, "echo %s > %s", imsi, IMSI_TMP_FILE);
            system(tmp);
        }
        else
        {
            snprintf(tmp, 255, "touch %s", IMSI_TMP_FILE);
            system(tmp);
        }
        
    }
    else if (NULL != strstr(cmd, ICCID_CMD))
    {
        /* 获取iccid */
        memcpy(iccid, buf, ICCID_LEN);
        iccid[ICCID_LEN] = '\0';
        
        for (i = 0; i < ICCID_LEN; i++)
        {
            if (!(('0' <= iccid[i] && '9' >= iccid[i]) || 
                ('a' <= iccid[i] && 'z' >= iccid[i]) || 
                ('A' <= iccid[i] && 'Z' >= iccid[i])))
            {
                flag = NOT_WRITE_FILE;
                break;
            }
        }
        if (WRITE_FILE == flag)
        {
            snprintf(tmp, 255, "echo %s > %s", iccid, ICCID_TMP_FILE);
            system(tmp);
        }
        else
        {
            snprintf(tmp, 255, "touch %s", ICCID_TMP_FILE);
			system(tmp);
        }
        
        
    }
    else if (NULL != strstr(cmd, SIM_STATUS_CMD))
    {
        /* 获取网卡状态 */
        snprintf(tmp, 255, "echo %s > %s", buf, CPIN_TMP_FILE);
        system(tmp);
    }
    else if (NULL != strstr(cmd, SIGNAL_STRENGTH_CMD))
    {
        /* 获取信号强度 */
        snprintf(tmp, 255, "echo %s > %s", buf, CSQ_TMP_FILE);
        system(tmp);
    }
    else if (NULL != strstr(cmd, OPEN_GPS_CMD))
    {
        /* 获取gps 开启结果 */
        snprintf(tmp, 255, "echo %s > %s", buf, OPEN_GPS_TMP_FILE);
        system(tmp);
    }
    else if (NULL != strstr(cmd, NETWORK_CMD))
    {
        /* 获取网络模式 */
        snprintf(tmp, 255, "echo %c > %s", buf[0], NETWORK_FILE);
        system(tmp);
    }
    else if (NULL != strstr(cmd, SET_GET_BASE_STA))
    {
        /* 获取设置获取基站信息结果 */
        snprintf(tmp, 255, "echo %c > %s", buf[0], SET_BASE_STA_FILE);
        system(tmp);
    }
    else if (NULL != strstr(cmd, GET_BASE_STA))
    {
        /* 获取基站数据 */
        fd = fopen(BASE_STA_INFO_FILE, "w");
        if (NULL != fd)
        {
            fwrite(buf, strlen(buf), 1, fd);
            fclose(fd);
        }
    }

    return ;
}

void Rsu_Reset(void){
    //硬件复位模块
  system("echo none > /sys/devices/platform/leds-gpio/leds/ap147:green:wlan/trigger");
  system("echo 1 > /sys/devices/platform/leds-gpio/leds/ap147:green:wlan/brightness");
  usleep(300*1000);
  system("echo 0 > /sys/devices/platform/leds-gpio/leds/ap147:green:wlan/brightness");  
  sleep(1);
}


void *reset_monitor_routine(void *arg) {
    int fl;
    sleep(2);
    if(pthread_mutex_lock(&mutex)) {
        perror("lock mutex");
        return (void *)AT_ERROR_LOCK_MUTEX_FAILED;
    }
    fl = flag_step1_finished;
    if(pthread_mutex_unlock(&mutex)) {
        perror("unlock mutex");
        return (void *)AT_ERROR_UNLOCK_MUTEX_FAILED;
    }

    if(!fl) {
        fprintf(stderr, "=====> no response, reset mcu\n");
        Rsu_Reset();
    }
    return NULL;
}

int boot_reset_monitor(void) {
    pthread_t pid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8*1024);
    if(pthread_create(&pid, &attr, reset_monitor_routine, NULL)) {
        return AT_ERROR_CREATE_THREAD_FAILED;
    }
    pthread_detach(pid);
    return 0;
}

int redirect_stdout(void) {
    FILE *null_fp;
    int new_output = dup(fileno(stdout));

    if(new_output < 0) {
        perror("dup stdout");
	    return -1;
    }
    null_fp = fopen("/dev/null", "w");
    if(NULL == null_fp) {
        perror("fopen /dev/null");
    	return -1;
    }
    fclose(stdout);
    fclose(stderr);
    if(dup2(fileno(null_fp), STDOUT_FILENO) < 0) {
        perror("dup2 null_fp");
        return -1;
    }
    if(dup2(fileno(null_fp), STDERR_FILENO) < 0) {
        perror("dup2 null_fp");
        return -1;
    }
    fclose(null_fp);
    return new_output;
}

/*****************************************************************
    Function: main
    Description:主函数
    Input: 
        argc : 入参个数
        argv : 
            3个参数分别是:软件名、串口名、指令
            4个参数分别是:软件名、波特率、串口名、指令
    Output: 无
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
int uart_main(int argc, char **argv)
{
    int ret;
    int new_output;
    char command[128];
    char buf[64];
    serial_port_info dev_info;

    memset(&dev_info, 0, sizeof(serial_port_info));
    if (4 == argc && !strcmp(argv[3], "-q"))
    {
        dev_info.dev_name = argv[1];
        strcpy(dev_info.file, argv[2]);
        dev_info.baud_rate = 115200;
        if((new_output = redirect_stdout()) < 0) {
            return -1;
        }
    }
    else if (3 == argc)
    {
        dev_info.dev_name = argv[1];
        strcpy(dev_info.file, argv[2]);
        dev_info.baud_rate = 115200;
    }
    else {
        printf("usage: %s <dev name> <file> [-q]\n", argv[0]);
        return AT_ERROR_PARAMETER_ERROR;
    }

    printf("do_update_init..\n");
    ret = do_update_init(&dev_info);
    if(AT_OK != ret) {
        fprintf(stderr, "do_update_init error\n");
        dprintf(new_output, "error = %d\n", AT_ERROR_INIT_UART_FAILED);
        return AT_ERROR_INIT_UART_FAILED;
    }

    if(ret = boot_reset_monitor()) {
        fprintf(stderr, "boot_reset_monitor error\n");
        goto end;
    }

    printf("do_update..\n");
    //ret = do_update(&dev_info);
    if(AT_OK != ret) {
        fprintf(stderr, "do_update error\n");
        //ret = AT_ERROR_UPDATE_FAILED;
    } else {
        printf("------------> succeed!\n");
    }

end:
    /* destroy */
    if(dev_info.file_fp) {
        printf("close file\n");
        fclose(dev_info.file_fp);
    }
    printf("close uart device file\n");
    UART_Close(dev_info.uart_fd);
    if(0 == ret) {
        dprintf(new_output, "success\n");
    } else {
        dprintf(new_output, "error = %d\n", ret);
    }
    return ret;
}

