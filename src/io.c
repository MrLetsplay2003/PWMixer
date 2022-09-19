#include <stdio.h>
#include <spa/param/audio/format-utils.h>

#include "pwmixer.h"

static const struct pw_stream_events inputStreamEvents = {
	PW_VERSION_STREAM_EVENTS,
	.process = pwm_ioProcessInput
};

static const struct pw_stream_events outputStreamEvents = {
	PW_VERSION_STREAM_EVENTS,
	.process = pwm_ioProcessOutput
};

void pwm_ioProcessInput(void *data) {
	pwm_Input *input = data;
	//printf("PROCESS in stream: %p\n", input->stream);

	struct pw_buffer *buf = pw_stream_dequeue_buffer(input->stream);
	if(!buf) return;

	for(uint32_t i = 0; i < input->outputCount; i++) {
		pwm_Output *out = input->outputs[i];
		while(!out->bufferAvailable); // TODO: ?
		out->bufferAvailable = false;
		size_t nWrite = SPA_MIN(PWM_OUTPUT_BUFFER_SIZE - out->bufferSize, buf->buffer->datas[0].chunk->size);
		memcpy(out->buffer + out->bufferSize, buf->buffer->datas[0].data, nWrite);
		out->bufferSize += nWrite;
		out->bufferAvailable = true;
	}

	pw_stream_queue_buffer(input->stream, buf);
}

void pwm_ioProcessOutput(void *data) {
	pwm_Output *output = data;
	//printf("PROCESS out stream: %p\n", output->stream);

	struct pw_buffer *buf = pw_stream_dequeue_buffer(output->stream);
	if(!buf) return;

	uint8_t *streamDat;
	if ((streamDat = buf->buffer->datas[0].data) == NULL)
		return;

	while(!output->bufferAvailable); // TODO: ?
	output->bufferAvailable = false;
	uint32_t stride = sizeof(float) * 2;
	uint32_t n_frames = SPA_MIN(buf->requested, SPA_MIN(buf->buffer->datas[0].maxsize / stride, output->bufferSize / stride));

	memcpy(streamDat, output->buffer, n_frames * stride);
	output->bufferSize -= n_frames * stride;
	//printf("Write %i frames buf sz: %i\n", n_frames, output->bufferSize);
	memmove(output->buffer, output->buffer + n_frames * stride, n_frames * stride); // Move buffer
	output->bufferAvailable = true;

	buf->buffer->datas[0].chunk->offset = 0;
	buf->buffer->datas[0].chunk->stride = stride;
	buf->buffer->datas[0].chunk->size = n_frames * stride;

	pw_stream_queue_buffer(output->stream, buf);
}

pwm_Input *pwm_ioCreateInput(const char *name) {
	printf("CREATE: %s\n", name);
	//pw_loop_signal_event(pw_main_loop_get_loop(data->mainLoop), NULL);

	struct pw_properties *streamProps = pw_properties_new(
		PW_KEY_MEDIA_TYPE, "Audio",
		PW_KEY_MEDIA_CATEGORY, "Capture",
		PW_KEY_MEDIA_ROLE, "Music",
		NULL);

	pwm_Input *input = malloc(sizeof(pwm_Input));
	input->outputCount = 0;
	input->outputs = NULL;

	struct pw_stream *stream = pw_stream_new_simple(pw_main_loop_get_loop(pwm_data->mainLoop), name, streamProps, &inputStreamEvents, input);
	printf("STREAM: %p\n", stream);
	input->stream = stream;

	uint8_t buf[1024];
	struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
	const struct spa_pod *params[1];
	params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &SPA_AUDIO_INFO_RAW_INIT(
		.format = SPA_AUDIO_FORMAT_F32,
		.channels = 2,
		.rate = 44100
	));
	pw_stream_connect(stream,
		PW_DIRECTION_INPUT,
		PW_ID_ANY,
		PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS /*| PW_STREAM_FLAG_RT_PROCESS*/,
		params, 1);

	return input;
}

pwm_Output *pwm_ioCreateOutput(const char *name) {
	printf("OUTCREATE: %s\n", name);
	//pw_loop_signal_event(pw_main_loop_get_loop(data->mainLoop), NULL);

	struct pw_properties *streamProps = pw_properties_new(
		PW_KEY_MEDIA_TYPE, "Audio",
		PW_KEY_MEDIA_CATEGORY, "Capture",
		PW_KEY_MEDIA_ROLE, "Music",
		NULL);

	pwm_Output *output = malloc(sizeof(pwm_Output));
	output->buffer = malloc(PWM_OUTPUT_BUFFER_SIZE);
	output->bufferSize = 0;
	output->bufferAvailable = true;

	struct pw_stream *stream = pw_stream_new_simple(pw_main_loop_get_loop(pwm_data->mainLoop), name, streamProps, &outputStreamEvents, output);
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
		PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS /*| PW_STREAM_FLAG_RT_PROCESS*/,
		params, 1);

	return output;
}

void pwm_ioConnect(pwm_Input *in, pwm_Output *out) {
	printf("CONNECT %p and %p\n", in, out);
	if(in->outputCount == 0) {
		in->outputs = malloc(sizeof(pwm_Output *));
	}else {
		in->outputs = realloc(in->outputs, (in->outputCount + 1) * sizeof(pwm_Output *));
	}

	in->outputs[in->outputCount++] = out;
}