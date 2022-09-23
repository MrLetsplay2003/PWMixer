#include <stdio.h>

#include "pwmixer.h"
#include "internal.h"

pwm_Data *pwm_data;

void pwm_sysConnect(int argc, char **argv) {
	pw_init(&argc, &argv);

	pwm_data = calloc(1, sizeof(pwm_Data));
	pwm_data->mainLoop = pw_main_loop_new(NULL);
	pwm_data->context = pw_context_new(pw_main_loop_get_loop(pwm_data->mainLoop), NULL, 0);
	pwm_data->core = pw_context_connect(pwm_data->context, NULL, 0);

	printf("Connected to PipeWire\n");
	pw_loop_add_signal(pw_main_loop_get_loop(pwm_data->mainLoop), SIGINT, pwm_sysDisconnect, NULL);
	pw_loop_add_signal(pw_main_loop_get_loop(pwm_data->mainLoop), SIGTERM, pwm_sysDisconnect, NULL);
	pwm_data->eventSource = pw_loop_add_event(pw_main_loop_get_loop(pwm_data->mainLoop), pwm_sysHandleEvent, NULL);

	pthread_mutex_init(&pwm_data->mutex, NULL);
	pwm_sysRun();
}

void pwm_sysRun() {
	int err;
	if((err = pthread_create(&pwm_data->thread, NULL, pwm_sysRunThread, NULL))) {
		pwm_data->thread = 0;
		printf("Failed to create thread: %i\n", err);
	}
}

void pwm_sysDisconnect() {
	printf("PWMixer exiting\n");
	pw_main_loop_quit(pwm_data->mainLoop);
}

void pwm_sysHandleEvent(void *data, uint64_t count) {
	pthread_mutex_lock(&pwm_data->mutex);
	printf("Got %lu events\n", count);

	for(uint32_t i = 0; i < pwm_data->eventCount; i++) {
		pwm_Event *event = pwm_data->events[i];
		switch(event->type) {
			case PWM_EVENT_CREATE_INPUT:
			{
				pwm_EventCreate *create = event->data;
				pwm_ioCreateInput0(create->object);
				break;
			}
			case PWM_EVENT_CREATE_OUTPUT:
			{
				pwm_EventCreate *create = event->data;
				pwm_ioCreateOutput0(create->object);
				break;
			}
			case PWM_EVENT_CONNECT:
			{
				pwm_EventConnect *connect = event->data;
				pwm_ioConnect0(connect->in, connect->out);
				break;
			}
			case PWM_EVENT_DISCONNECT:
			{
				pwm_EventConnect *connect = event->data;
				pwm_ioDisconnect0(connect->in, connect->out);
				break;
			}
			case PWM_EVENT_DESTROY:
			{
				pwm_EventDestroy *destroy = event->data;
				pwm_ioDestroy0(destroy->object);
				break;
			}
			default:
			{
				fprintf(stderr, "Got invalid event type %i, ignoring\n", event->type);
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

void pwm_sysCleanup() {
	printf("PWMixer cleaning up\n");

	for(uint32_t i = 0; i < pwm_data->objectCount; i++) {
		pwm_ioFree(pwm_data->objects[i]);
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
	printf("Running\n");
	pw_main_loop_run(pwm_data->mainLoop);
	printf("Exiting\n");
	pwm_sysCleanup();
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