#include <stdio.h>
#include <spa/param/audio/format-utils.h>
#include <pthread.h>

#include "pwmixer.h"
#include "internal.h"

static void streamStateChanged(void *data, enum pw_stream_state old, enum pw_stream_state state, const char *error) {
	printf("%p changed state from %s to %s: %s\n", data, pw_stream_state_as_string(old), pw_stream_state_as_string(state), error);
}

static const struct pw_stream_events inputStreamEvents = {
	PW_VERSION_STREAM_EVENTS,
	.process = pwm_ioProcessInput,
	.state_changed = streamStateChanged
};

static const struct pw_stream_events outputStreamEvents = {
	PW_VERSION_STREAM_EVENTS,
	.process = pwm_ioProcessOutput
};

void pwm_ioProcessInput(void *data) {
	pwm_IO *input = data;

	struct pw_buffer *buf = pw_stream_dequeue_buffer(input->stream);
	if(!buf) {
		printf("NO BUF\n");
		return;
	}

	for(uint32_t i = 0; i < input->connectionCount; i++) {
		pwm_Connection *con = input->connections[i];
		size_t nWrite = SPA_MIN(PWM_BUFFER_SIZE - con->bufferSize, buf->buffer->datas[0].chunk->size);

		memcpy(con->buffer + con->bufferSize, buf->buffer->datas[0].data, nWrite);
		con->bufferSize += nWrite;
	}

	pw_stream_queue_buffer(input->stream, buf);
}

void pwm_ioProcessOutput(void *data) {
	pwm_IO *output = data;

	struct pw_buffer *buf = pw_stream_dequeue_buffer(output->stream);
	if(!buf) return;

	if(output->connectionCount == 0 || buf->requested == 0) {
		pw_stream_queue_buffer(output->stream, buf);
		return;
	}

	uint8_t *streamDat;
	if ((streamDat = buf->buffer->datas[0].data) == NULL)
		return;

	uint32_t stride = sizeof(float) * PWM_CHANNELS;
	uint32_t n_frames = SPA_MIN(buf->requested, buf->buffer->datas[0].maxsize / stride);

	for(uint32_t i = 0; i < output->connectionCount; i++) {
		pwm_Connection *con = output->connections[i];
		n_frames = SPA_MIN(con->bufferSize / stride, n_frames);
	}

	uint32_t nRead = n_frames * stride;

	memset(streamDat, 0, nRead);

	for(uint32_t i = 0; i < output->connectionCount; i++) {
		pwm_Connection *con = output->connections[i];

		for(size_t i = 0; i < n_frames * PWM_CHANNELS; i++) {
			*(((float *) streamDat) + i) += *(((float *) con->buffer) + i);
		}

		con->bufferSize -= nRead;
		memmove(con->buffer, con->buffer + nRead, con->bufferSize); // Move remaining buffer
	}

	buf->buffer->datas[0].chunk->offset = 0;
	buf->buffer->datas[0].chunk->stride = stride;
	buf->buffer->datas[0].chunk->size = n_frames * stride;

	pw_stream_queue_buffer(output->stream, buf);
}

void pwm_ioFree(pwm_IO *object) {
	free(object->name);
	free(object);
}

void pwm_ioFreeConnection(pwm_Connection *connection) {
	free(connection);
}

void pwm_ioCreateInput0(pwm_IO *input) {
	struct pw_properties *streamProps;
	if(input->isSourceOrSink) {
		streamProps = pw_properties_new(
			PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Capture",
			PW_KEY_MEDIA_ROLE, "Music",
			PW_KEY_MEDIA_CLASS, "Audio/Sink",
			PW_KEY_NODE_NAME, input->name,
			PW_KEY_NODE_NICK, input->name,
			PW_KEY_NODE_DESCRIPTION, input->name,
			NULL);
	}else {
		streamProps = pw_properties_new(
			PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Capture",
			PW_KEY_MEDIA_ROLE, "Music",
			NULL);
	}

	input->connections = NULL;
	input->connectionCount = 0;

	struct pw_stream *stream = pw_stream_new(pwm_data->core, input->name, streamProps);
	pw_stream_add_listener(stream, &input->hook, &inputStreamEvents, input);
	input->stream = stream;

	uint8_t buf[1024];
	struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
	const struct spa_pod *params[1];
	params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &SPA_AUDIO_INFO_RAW_INIT(
		.format = SPA_AUDIO_FORMAT_F32,
		.channels = PWM_CHANNELS,
		.rate = PWM_RATE
	));
	pw_stream_connect(stream,
		PW_DIRECTION_INPUT,
		PW_ID_ANY,
		PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS,
		params, 1);
}

void pwm_ioCreateOutput0(pwm_IO *output) {
	struct pw_properties *streamProps;
	if(output->isSourceOrSink){
		streamProps = pw_properties_new(
			PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Capture",
			PW_KEY_MEDIA_ROLE, "Music",
			PW_KEY_MEDIA_CLASS, "Audio/Source",
			PW_KEY_NODE_NAME, output->name,
			PW_KEY_NODE_NICK, output->name,
			PW_KEY_NODE_DESCRIPTION, output->name,
			NULL);
	}else {
		streamProps = pw_properties_new(
			PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Capture",
			PW_KEY_MEDIA_ROLE, "Music",
			NULL);
	}

	output->connections = NULL;
	output->connectionCount = 0;

	struct pw_stream *stream = pw_stream_new(pwm_data->core, output->name, streamProps);
	pw_stream_add_listener(stream, &output->hook, &outputStreamEvents, output);
	output->stream = stream;

	uint8_t buf[1024];
	struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
	const struct spa_pod *params[1];
	params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &SPA_AUDIO_INFO_RAW_INIT(
		.format = SPA_AUDIO_FORMAT_F32,
		.channels = 2,
		.rate = 44100
	));
	pw_stream_connect(stream,
		PW_DIRECTION_OUTPUT,
		PW_ID_ANY,
		PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS,
		params, 1);
}

void pwm_ioDestroy0(pwm_IO *object) {
	for(uint32_t i = 0; i < object->connectionCount; i++) {
		pwm_Connection *con = object->connections[i];
		pwm_ioDisconnect0(con->input, con->output);
	}

	for(uint32_t i = 0; i < pwm_data->objectCount; i++) {
		pwm_IO *o = pwm_data->objects[i];
		if(o == object) {
			pwm_data->objects[i] = pwm_data->objects[--pwm_data->objectCount];
			pwm_data->objects = realloc(pwm_data->objects, pwm_data->objectCount * sizeof(pwm_IO *));
		}
	}

	spa_hook_remove(&object->hook);
	pw_stream_destroy(object->stream);
	pwm_ioFree(object);
}

static void pwm_ioAddConnection(pwm_Connection ***arr, uint32_t *count, pwm_Connection *con) {
	if(*count == 0) {
		*arr = malloc(sizeof(pwm_Connection *));
	}else {
		*arr = realloc(*arr, (*count + 1) * sizeof(pwm_Connection *));
	}

	(*arr)[*count] = con;
	*count += 1;
}

void pwm_ioConnect0(pwm_IO *in, pwm_IO *out) {
	printf("Connecting %p (%s) and %p (%s)\n", in, in->name, out, out->name);

	pwm_Connection *con = malloc(sizeof(pwm_Connection));
	con->input = in;
	con->output = out;
	con->buffer = malloc(PWM_BUFFER_SIZE);
	con->bufferSize = 0;

	pwm_ioAddConnection(&in->connections, &in->connectionCount, con);
	pwm_ioAddConnection(&out->connections, &out->connectionCount, con);
}

void pwm_ioDisconnect0(pwm_IO *in, pwm_IO *out) {
	pwm_Connection *connection;
	for(uint32_t i = 0; i < in->connectionCount; i++) {
		pwm_Connection *con = in->connections[i];
		if(con->input == in && con->output == out) {
			connection = con;
			break;
		}
	}

	if(connection == NULL) return;

	for(uint32_t i = 0; i < in->connectionCount; i++) {
		pwm_Connection *con = in->connections[i];
		if(con == connection) {
			in->connections[i] = in->connections[--in->connectionCount];
			in->connections = realloc(in->connections, in->connectionCount * sizeof(pwm_Connection *));
			break;
		}
	}

	for(uint32_t i = 0; i < out->connectionCount; i++) {
		pwm_Connection *con = out->connections[i];
		if(con == connection) {
			out->connections[i] = out->connections[--out->connectionCount];
			out->connections = realloc(out->connections, out->connectionCount * sizeof(pwm_Connection *));
			break;
		}
	}

	free(connection);
}

void pwm_ioConnect(pwm_IO *input, pwm_IO *output) {
	pwm_EventConnect *connect = malloc(sizeof(pwm_EventConnect));
	connect->in = input;
	connect->out = output;

	pwm_Event *event = malloc(sizeof(pwm_Event));
	event->type = PWM_EVENT_CONNECT;
	event->data = connect;

	pwm_sysEnqueueEvent(event);
}

void pwm_ioDisconnect(pwm_IO *input, pwm_IO *output) {
	pwm_EventConnect *connect = malloc(sizeof(pwm_EventConnect));
	connect->in = input;
	connect->out = output;

	pwm_Event *event = malloc(sizeof(pwm_Event));
	event->type = PWM_EVENT_DISCONNECT;
	event->data = connect;

	pwm_sysEnqueueEvent(event);
}

pwm_IO *pwm_ioCreateInput(const char *name, bool isSink) {
	pwm_EventCreate *create = malloc(sizeof(pwm_EventCreate));

	size_t nameLen = strlen(name);
	char *nameChars = malloc(nameLen + 1);
	memcpy(nameChars, name, nameLen);
	nameChars[nameLen] = 0;

	pwm_IO *object = calloc(1, sizeof(pwm_IO));
	object->name = nameChars;
	object->isSourceOrSink = isSink;
	create->object = object;

	pwm_data->objects = realloc(pwm_data->objects, (pwm_data->objectCount + 1) * sizeof(pwm_IO *));
	pwm_data->objects[pwm_data->objectCount++] = object;

	pwm_Event *event = malloc(sizeof(pwm_Event));
	event->type = PWM_EVENT_CREATE_INPUT;
	event->data = create;

	pwm_sysEnqueueEvent(event);

	return object;
}

pwm_IO *pwm_ioCreateOutput(const char *name, bool isSource) {
	pwm_EventCreate *create = malloc(sizeof(pwm_EventCreate));

	size_t nameLen = strlen(name);
	char *nameChars = malloc(nameLen + 1);
	memcpy(nameChars, name, nameLen);
	nameChars[nameLen] = 0;

	pwm_IO *object = calloc(1, sizeof(pwm_IO));
	object->name = nameChars;
	object->isSourceOrSink = isSource;
	create->object = object;

	pwm_data->objects = realloc(pwm_data->objects, (pwm_data->objectCount + 1) * sizeof(pwm_IO *));
	pwm_data->objects[pwm_data->objectCount++] = object;

	pwm_Event *event = malloc(sizeof(pwm_Event));
	event->type = PWM_EVENT_CREATE_OUTPUT;
	event->data = create;

	pwm_sysEnqueueEvent(event);

	return object;
}

uint32_t pwm_ioGetID(pwm_IO *object) {
	return object->id;
}

pwm_IO *pwm_ioGetByID(uint32_t id){
	return pwm_data->objects[id];
}

void pwm_ioDestroy(pwm_IO *object) {
	pwm_EventDestroy *destroy = malloc(sizeof(pwm_EventDestroy));
	destroy->object = object;

	pwm_Event *event = malloc(sizeof(pwm_Event));
	event->type = PWM_EVENT_DESTROY;
	event->data = destroy;

	pwm_sysEnqueueEvent(event);
}