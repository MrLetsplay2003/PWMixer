#include <stdio.h>
#include <spa/param/audio/format-utils.h>
#include <pthread.h>

#include "pwmixer.h"
#include "types.h"

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
	pwm_Input *input = data;

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
	pwm_Output *output = data;

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

pwm_Input *pwm_ioCreateInput(const char *name, bool isSink) {
	struct pw_properties *streamProps;
	if(isSink) {
		streamProps = pw_properties_new(
			PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Capture",
			PW_KEY_MEDIA_ROLE, "Music",
			PW_KEY_MEDIA_CLASS, "Audio/Sink",
			PW_KEY_NODE_NAME, name,
			PW_KEY_NODE_NICK, name,
			PW_KEY_NODE_DESCRIPTION, name,
			NULL);
	}else {
		streamProps = pw_properties_new(
			PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Capture",
			PW_KEY_MEDIA_ROLE, "Music",
			NULL);
	}


	pwm_Input *input = malloc(sizeof(pwm_Input));
	input->connections = NULL;
	input->connectionCount = 0;

	struct pw_stream *stream = pw_stream_new(pwm_data->core, name, streamProps);
	pw_stream_add_listener(stream, &input->hook, &inputStreamEvents, input);
	printf("STREAM: %p\n", stream);
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

	if(pwm_data->inputCount == 0) {
		pwm_data->inputs = malloc(sizeof(pwm_Input *));
	}else {
		pwm_data->inputs = realloc(pwm_data->inputs, (pwm_data->inputCount + 1) * sizeof(pwm_Input *));
	}

	pwm_data->inputs[pwm_data->inputCount++] = input;

	return input;
}

pwm_Output *pwm_ioCreateOutput(const char *name, bool isSource) {
	printf("OUTCREATE: %s\n", name);

	struct pw_properties *streamProps;
	if(isSource){
		streamProps = pw_properties_new(
			PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Capture",
			PW_KEY_MEDIA_ROLE, "Music",
			PW_KEY_MEDIA_CLASS, "Audio/Source",
			PW_KEY_NODE_NAME, name,
			PW_KEY_NODE_NICK, name,
			PW_KEY_NODE_DESCRIPTION, name,
			NULL);
	}else {
		streamProps = pw_properties_new(
			PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Capture",
			PW_KEY_MEDIA_ROLE, "Music",
			NULL);
	}

	pwm_Output *output = malloc(sizeof(pwm_Output));
	output->connections = NULL;
	output->connectionCount = 0;

	struct pw_stream *stream = pw_stream_new(pwm_data->core, name, streamProps);
	pw_stream_add_listener(stream, &output->hook, &outputStreamEvents, output);
	printf("OUTSTREAM: %p\n", stream);
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

	if(pwm_data->outputCount == 0) {
		pwm_data->outputs = malloc(sizeof(pwm_Output *));
	}else {
		pwm_data->outputs = realloc(pwm_data->outputs, (pwm_data->outputCount + 1) * sizeof(pwm_Output *));
	}

	pwm_data->outputs[pwm_data->outputCount++] = output;

	return output;
}

void pwm_ioDestroyOutput(pwm_Output *output) {
	for(uint32_t i = 0; i < output->connectionCount; i++) {
		pwm_Connection *con = output->connections[i];
		pwm_ioDisconnect(con->input, con->output);
	}

	spa_hook_remove(&output->hook);
	pw_stream_destroy(output->stream);
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

void pwm_ioConnect(pwm_Input *in, pwm_Output *out) {
	printf("CONNECT %p and %p\n", in, out);

	pwm_Connection *con = malloc(sizeof(pwm_Connection));
	con->input = in;
	con->output = out;
	con->buffer = malloc(PWM_BUFFER_SIZE);
	con->bufferSize = 0;

	pwm_ioAddConnection(&in->connections, &in->connectionCount, con);
	pwm_ioAddConnection(&out->connections, &out->connectionCount, con);
}

void pwm_ioDisconnect(pwm_Input *in, pwm_Output *out) {
	// TODO: implement
}