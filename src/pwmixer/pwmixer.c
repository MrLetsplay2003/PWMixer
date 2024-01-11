#include <stdio.h>

#include "internal.h"
#include "pwmixer.h"

pwm_Data *pwm_data;
static bool pwm_log = false;

int pwm_sysConnect(int *argc, char ***argv) {
	int err = PWM_ERROR_NONE;

	pw_init(argc, argv);

	pwm_data = calloc(1, sizeof(pwm_Data));
	if(!pwm_data) return PWM_ERROR_MEM;

	pwm_data->mainLoop = pw_main_loop_new(NULL);
	if(!pwm_data->mainLoop) {
		err = PWM_ERROR_PIPEWIRE;
		goto cleanup_data;
	}

	pwm_data->context = pw_context_new(pw_main_loop_get_loop(pwm_data->mainLoop), NULL, 0);
	if(!pwm_data->context) {
		err = PWM_ERROR_PIPEWIRE;
		goto cleanup_mainloop;
	}

	pwm_data->core = pw_context_connect(pwm_data->context, NULL, 0);
	if(!pwm_data->context) {
		err = PWM_ERROR_PIPEWIRE;
		goto cleanup_context;
	}

	if(pwm_debugIsLogEnabled()) printf("Connected to PipeWire\n");
	pwm_data->eventSource = pw_loop_add_event(pw_main_loop_get_loop(pwm_data->mainLoop), pwm_sysHandleEvent, NULL);
	pwm_data->quitEventSource = pw_loop_add_event(pw_main_loop_get_loop(pwm_data->mainLoop), pwm_sysHandleExit, NULL);
	if(!pwm_data->eventSource || !pwm_data->quitEventSource) {
		err = PWM_ERROR_PIPEWIRE;
		goto cleanup_context;
	}

	if(pthread_mutex_init(&pwm_data->mutex, NULL)) {
		err = PWM_ERROR_OTHER;
		goto cleanup_context;
	}

	int pthreadErr;
	if((pthreadErr = pthread_create(&pwm_data->thread, NULL, pwm_sysRunThread, NULL))) {
		if(pwm_debugIsLogEnabled()) printf("Failed to create thread: %i\n", err);
		err = PWM_ERROR_OTHER;
		goto cleanup_context;
	}

	return PWM_ERROR_NONE;

cleanup_context:
	pw_context_destroy(pwm_data->context);
cleanup_mainloop:
	pw_main_loop_destroy(pwm_data->mainLoop);
cleanup_data:
	free(pwm_data);
	pwm_data = NULL;
	return err;
}

void pwm_sysDisconnect() {
	if(!pwm_data) return;
	pw_loop_signal_event(pw_main_loop_get_loop(pwm_data->mainLoop), pwm_data->quitEventSource);

	int err;
	if((err = pthread_join(pwm_data->thread, NULL))) {
		if(pwm_debugIsLogEnabled()) printf("Failed to join: %i\n", err);
		return;
	}
}

bool pwm_sysIsRunning() {
	return pwm_data != NULL;
}

void pwm_sysHandleEvent(void *data, uint64_t count) {
	pthread_mutex_lock(&pwm_data->mutex);
	if(pwm_debugIsLogEnabled()) printf("Got %lu events\n", count);

	for(uint32_t i = 0; i < pwm_data->eventCount; i++) {
		pwm_Event *event = pwm_data->events[i];
		switch(event->type) {
			case PWM_EVENT_CREATE_INPUT: {
				pwm_EventCreate *create = event->data;
				pwm_ioCreateInput0(create->object);
				break;
			}
			case PWM_EVENT_CREATE_OUTPUT: {
				pwm_EventCreate *create = event->data;
				pwm_ioCreateOutput0(create->object);
				break;
			}
			case PWM_EVENT_CONNECT: {
				pwm_EventConnect *connect = event->data;
				pwm_ioConnect0(connect->in, connect->out);
				break;
			}
			case PWM_EVENT_DISCONNECT: {
				pwm_EventConnect *connect = event->data;
				pwm_ioDisconnect0(connect->in, connect->out);
				break;
			}
			case PWM_EVENT_DESTROY: {
				pwm_EventDestroy *destroy = event->data;
				pwm_ioDestroy0(destroy->object);
				break;
			}
			case PWM_EVENT_SET_CONNECTION_VOLUME: {
				pwm_EventSetConnectionVolume *setVolume = event->data;
				pwm_Connection *con = pwm_ioGetConnection(setVolume->in, setVolume->out);
				if(!con) break;
				con->volume = setVolume->volume;
				break;
			}
			default: {
				if(pwm_debugIsLogEnabled()) fprintf(stderr, "Got invalid event type %i, ignoring\n", event->type);
				break;
			}
		}
		pwm_sysFreeEvent(event);
	}

	free(pwm_data->events);
	pwm_data->events = NULL;
	pwm_data->eventCount = 0;

	pthread_mutex_unlock(&pwm_data->mutex);
}

void pwm_sysHandleExit() {
	pw_main_loop_quit(pwm_data->mainLoop);
}

void pwm_sysCleanup() {
	if(pwm_debugIsLogEnabled()) printf("PWMixer cleaning up\n");

	for(uint32_t i = 0; i < pwm_data->objectCount; i++) {
		pwm_IO *object = pwm_data->objects[i];
		if(object) pwm_ioFree(object);
	}

	for(uint32_t i = 0; i < pwm_data->eventCount; i++) {
		pwm_sysFreeEvent(pwm_data->events[i]);
	}

	pw_core_disconnect(pwm_data->core);
	pw_context_destroy(pwm_data->context);
	pw_main_loop_destroy(pwm_data->mainLoop);
	pw_deinit();
	pthread_mutex_destroy(&pwm_data->mutex);

	free(pwm_data);
	pwm_data = NULL;
}

void *pwm_sysRunThread(void *data) {
	sigset_t sigmask;
	sigemptyset(&sigmask);
	pthread_sigmask(SIG_SETMASK, &sigmask, NULL);

	if(pwm_debugIsLogEnabled()) printf("PWMixer is running\n");
	pw_main_loop_run(pwm_data->mainLoop);
	if(pwm_debugIsLogEnabled()) printf("PWMixer is exiting\n");
	pwm_sysCleanup();
	if(pwm_debugIsLogEnabled()) printf("PWMixer exit\n");
	return NULL;
}

void pwm_sysEnqueueEvent(pwm_Event *event) {
	pthread_mutex_lock(&pwm_data->mutex);

	pwm_data->events = realloc(pwm_data->events, (pwm_data->eventCount + 1) * sizeof(pwm_Event *));
	pwm_data->events[pwm_data->eventCount++] = event;

	pthread_mutex_unlock(&pwm_data->mutex);

	pw_loop_signal_event(pw_main_loop_get_loop(pwm_data->mainLoop), pwm_data->eventSource);
}

void pwm_sysFreeEvent(pwm_Event *event) {
	free(event->data);
	free(event);
}

void pwm_debugEnableLog(bool log) {
	pwm_log = log;
}

bool pwm_debugIsLogEnabled() {
	return pwm_log;
}
