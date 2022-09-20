#ifndef _PWMIXER_H_
#define _PWMIXER_H_

#include <pipewire/pipewire.h>
#include <stdint.h>
#include <stdatomic.h>

#define PWM_BUFFER_SIZE 8192

typedef struct pwm_Input pwm_Input;
typedef struct pwm_Output pwm_Output;
typedef struct pwm_Connection pwm_Connection;

struct pwm_Connection {
	pwm_Input *input;
	pwm_Output *output;

	atomic_bool bufferAvailable;
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

	pwm_Input *inputs;
	pwm_Output *outputs;
} pwm_Data;

extern pwm_Data *pwm_data;

void pwm_sysConnect(int argc, char **argv);
void pwm_sysRun();
void pwm_sysDisconnect();

void pwm_ioConnect(pwm_Input *input, pwm_Output *output);
void pwm_ioDisconnect(pwm_Input *input, pwm_Output *output);

pwm_Input *pwm_ioCreateInput(const char *name, bool isSink);
void pwm_ioProcessInput(void *data);

pwm_Output *pwm_ioCreateOutput(const char *name, bool isSource);
void pwm_ioProcessOutput(void *data);

#endif