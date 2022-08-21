/*
 * @Author: your name
 * @Date: 2021-09-01 20:52:35
 * @LastEditTime: 2021-09-02 11:23:50
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \smart_control\main\periph\ir.h
 */
#ifndef _IR_H_
#define _IR_H_
#include "stdint.h"
#include "stdbool.h"


uint8_t ac_set_code_lib(uint8_t band, uint8_t pro_code);
int ac_set_temp(int temp);
int ac_open(bool open);
int ac_set_wind_speed(int speed);
int ac_set_swing(bool open);
int ac_set_mode(int mode);
int ac_status_config(bool open,int temp,int speed);

int ac_get_temp();
int ac_get_power();
int ac_get_wind_speed();
int ac_get_mode();

void IR_init();  //初始化

#endif