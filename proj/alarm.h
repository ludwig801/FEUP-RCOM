#ifndef _ALARM_H_
#define _ALARM_H_

#include <signal.h>

void trigger_alarm();

void install_alarm();

void set_alarm_flag(int value);

int get_alarm_flag();

#endif


