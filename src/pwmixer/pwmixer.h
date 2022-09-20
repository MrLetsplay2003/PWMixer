#ifndef _PWMIXER_H_
#define _PWMIXER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pipewire/pipewire.h>
#include <stdint.h>

#define PWM_CHANNELS 2
#define PWM_RATE 44100

#define PWM_BUFFER_SIZE 8192

typedef struct pwm_Input pwm_Input;
typedef struct pwm_Output pwm_Output;
typedef struct pwm_Connection pwm_Connection;

struct pwm_Connection {
	pwm_Input *input;
	pwm_Output *output;

	uint8_t *buffer;
	size_t bufferSize;
};

struct pwm_Output {
	struct pw_stream *stream;
	struct spa_hook hook;

	pwm_Connection **connections;
	uint32_t connectionCount;
};

struct pwm_Input {
	struct pw_stream *stream;
	struct spa_hook hook;

	pwm_Connection **connections;
	uint32_t connectionCount;
};

typedef struct pwm_Data {
	struct pw_main_loop *mainLoop;
	struct pw_context *context;
	struct pw_core *core;

	struct {
		struct pw_node *node;
		struct pw_port **ports;
		uint32_t portCount;
	} *nodes;
	uint32_t nodeCount;

	pwm_Input **inputs;
	uint32_t inputCount;
	pwm_Output **outputs;
	uint32_t outputCount;
} pwm_Data;

extern pwm_Data *pwm_data;

void pwm_sysConnect(int argc, char **argv);
void pwm_sysRun();
void pwm_sysDisconnect();

void pwm_ioConnect(pwm_Input *input, pwm_Output *output);
void pwm_ioDisconnect(pwm_Input *input, pwm_Output *output);

pwm_Input *pwm_ioCreateInput(const char *name, bool isSink);
void pwm_ioDestroyInput(pwm_Input *input);
void pwm_ioProcessInput(void *data);

pwm_Output *pwm_ioCreateOutput(const char *name, bool isSource);
void pwm_ioDestroyOutput(pwm_Output *output);
void pwm_ioProcessOutput(void *data);

#ifdef __cplusplus
}
#endif

#endif