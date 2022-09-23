#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include "pwmixer.h"
#include "types.h"

extern pwm_Data *pwm_data;

void pwm_sysHandleEvent(void *data, uint64_t count);
void pwm_sysRun();
void pwm_sysCleanup();

void pwm_sysEnqueueEvent(pwm_Event *event);
void pwm_sysFreeEvent(pwm_Event *event);

void *pwm_sysRunThread(void *data);

void pwm_ioProcessInput(void *data);
void pwm_ioProcessOutput(void *data);
void pwm_ioFree(pwm_IO *object);

// Impls of the API functions
void pwm_ioConnect0(pwm_IO *input, pwm_IO *output);
void pwm_ioDisconnect0(pwm_IO *input, pwm_IO *output);

void pwm_ioCreateInput0(pwm_IO *input);
void pwm_ioCreateOutput0(pwm_IO *output);
void pwm_ioDestroy0(pwm_IO *object);

#endif