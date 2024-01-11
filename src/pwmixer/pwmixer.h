#ifndef _PWMIXER_H_
#define _PWMIXER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define PWM_CHANNELS 2
#define PWM_RATE 44100

#define PWM_BUFFER_SIZE 16384
#define PWM_INVALID_ID 0xFFFFFFFF

#define PWM_ERROR_NONE 0 // Everything is OK
#define PWM_ERROR_OTHER 1 // Got another error
#define PWM_ERROR_ARG 2 // Invalid arguments
#define PWM_ERROR_MEM 3 // Memory error, e.g. failed to allocate memory
#define PWM_ERROR_PIPEWIRE 4 // Got a PipeWire error

typedef struct pwm_IO pwm_IO;
typedef struct pwm_Connection pwm_Connection;
typedef struct pwm_Data pwm_Data;

typedef enum pwm_EventType pwm_EventType;
typedef struct pwm_Event pwm_Event;
typedef struct pwm_EventCreate pwm_EventCreate;
typedef struct pwm_EventDestroy pwm_EventDestroy;
typedef struct pwm_EventConnect pwm_EventConnect;
typedef struct pwm_EventSetConnectionVolume pwm_EventSetConnectionVolume;

int pwm_sysConnect(int *argc, char ***argv);
bool pwm_sysIsRunning();
void pwm_sysDisconnect();

void pwm_ioConnect(pwm_IO *input, pwm_IO *output);
void pwm_ioDisconnect(pwm_IO *input, pwm_IO *output);

pwm_IO *pwm_ioCreateInput(const char *name, bool isSink);
pwm_IO *pwm_ioCreateOutput(const char *name, bool isSource);
uint32_t pwm_ioGetID(pwm_IO *object);
pwm_IO *pwm_ioGetByID(uint32_t id);
void pwm_ioDestroy(pwm_IO *input);

void pwm_ioSetVolume(pwm_IO *object, float volume);
void pwm_ioSetConnectionVolume(pwm_IO *input, pwm_IO *output, float volume);

float pwm_ioGetLastVolume(pwm_IO *object);

void pwm_debugEnableLog(bool log);
bool pwm_debugIsLogEnabled();

#ifdef __cplusplus
}
#endif

#endif
