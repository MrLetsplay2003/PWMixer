#include <pipewire/pipewire.h>

#include "pwmixer.h"

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

struct pwm_Data {
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
};