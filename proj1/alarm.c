#include "alarm.h"

int flag = 1;

void trigger_alarm() {

	flag = 1;
}

void set_alarm_flag(int value) {

	flag = value;
}

int get_alarm_flag() {

	return flag;
}

void install_alarm() {

	// instala  rotina que atende interrupcao
	(void) signal(SIGALRM, trigger_alarm);
}

