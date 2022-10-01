#ifndef _TYPES_H_
#define _TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pipewire/pipewire.h>
#include <pthread.h>

#include "pwmixer.h"

struct pwm_Connection {
	pwm_IO *input;
	pwm_IO *output;

	uint8_t *buffer;
	size_t bufferSize;

	float volume;
};

struct pwm_IO {
	uint32_t id;

	char *name;
	bool isSourceOrSink;

	struct pw_stream *stream;
	struct spa_hook hook;

	pwm_Connection **connections;
	uint32_t connectionCount;

	float volume;
	float lastVolume; // Volume of the last chunk of audio data
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

	pwm_IO **objects;
	uint32_t objectCount;

	pthread_t thread;
	pthread_mutex_t mutex;

	pwm_Event **events;
	uint32_t eventCount;

	struct spa_source *eventSource;
	struct spa_source *quitEventSource;
};

enum pwm_EventType {
	PWM_EVENT_CREATE_INPUT,
	PWM_EVENT_CREATE_OUTPUT,
	PWM_EVENT_DESTROY,
	PWM_EVENT_CONNECT,
	PWM_EVENT_DISCONNECT,
	PWM_EVENT_SET_CONNECTION_VOLUME,
};

struct pwm_Event {
	pwm_EventType type;
	void *data;
};

struct pwm_EventCreate {
	pwm_IO *object;
};

struct pwm_EventDestroy {
	pwm_IO *object;
};

struct pwm_EventConnect {
	pwm_IO *in;
	pwm_IO *out;
};

struct pwm_EventSetConnectionVolume {
	pwm_IO *in;
	pwm_IO *out;
	float volume;
};

#ifdef __cplusplus
}
#endif

#endif