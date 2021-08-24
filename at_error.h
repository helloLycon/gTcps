/************************************************
    File name: at_error.h
    Author: 柳文剑
    Version:0.0.1
    Date:2018-4-19
    Description:错误码汇总
    Others:
*************************************************/
#ifndef __AT_ERROR_H__
#define __AT_ERROR_H__


/* 错误码 */
typedef enum error_num
{
    AT_OK = 0x0000,                                     /* 成功 */

    AT_ERROR_OPEN_FAILED = 0x0001,                      /* open 函数操作失败 */
    AT_ERROR_FCNTL_FAILED = 0x0002,                     /* fcntl 函数操作 失败 */
    AT_ERROR_TCGETATTR_FAILED = 0x0003,                 /* 获取串口参数失败 */
    AT_ERROR_PARAMETER_ERROR = 0x0004,                  /* 参数错误 */
    AT_ERROR_TCSETATTR_FAILED = 0x0005,                 /* 设置串口参数失败 */
    AT_ERROR_SELECT_FAILED = 0x0006,                    /* select 函数调用失败 */
    AT_ERROR_WRITE_FAILED = 0x0007,                     /* write 函数调用失败 */
    AT_ERROR_RECV_FAILED = 0x0008,                      /* 串口数据获取失败*/
    AT_ERROR_INIT_UART_FAILED = 9,
    AT_ERROR_UPDATE_FAILED    = 10,
    AT_ERROR_OPEN_FILE_FAILED = 11,
    AT_ERROR_READ_FILE_FAILED = 12,
    AT_ERROR_STEP_1_FAILED    = 13,
    AT_ERROR_SEND_FILE_LENGTH_FAILED = 14,
    AT_ERROR_FILE_CONTENT_FAILED     = 15,
    AT_ERROR_FILE_END_FAILED         = 16,
    AT_ERROR_ALL_FINISHED_FAILED     = 17,
    AT_ERROR_LOCK_MUTEX_FAILED     = 18,
    AT_ERROR_UNLOCK_MUTEX_FAILED     = 19,
    AT_ERROR_CREATE_THREAD_FAILED     = 20,
    
    AT_ERROR_MAX = 0xFFFF
}error_num;

#endif /*__CATCH_ERROR_H__*/

