#include <math.h>
#include <pwmixer.h>

#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void handleInterrupt(int sig) {
	pwm_sysDisconnect();
}

static uint64_t n;
static float interval = 2.0f;

static void doFilter(float *samples, size_t sampleCount) {
	for(size_t i = 0; i < sampleCount * PWM_CHANNELS; i += PWM_CHANNELS) {
		float f = n * M_PI / (interval * 44100.0f);
		samples[i] *= sin(f) * 0.5f + 0.5f;
		samples[i + 1] *= -sin(f) * 0.5f + 0.5f;
		n++;
	}
}

#define BUFFER_SIZE (PWM_CHANNELS * 44100 / 2)
static float buffer[BUFFER_SIZE];
static size_t bufferIndex = BUFFER_SIZE;

static void doFilter2(float *samples, size_t sampleCount) {
	size_t totalSamples = sampleCount * PWM_CHANNELS;
	printf("%zu\n", bufferIndex);

	size_t echoSamples = bufferIndex < totalSamples ? bufferIndex : totalSamples;
	printf("SAMPLES: %zu / %zu\n", echoSamples, totalSamples);
	for(size_t i = 0; i < echoSamples; i++) {
		samples[i] += buffer[i] * 0.5f;
	}

	memmove(buffer, buffer + echoSamples, (bufferIndex - echoSamples) * sizeof(float));
	bufferIndex -= echoSamples;

	size_t freeSpace = BUFFER_SIZE - bufferIndex;
	size_t toCopy = totalSamples < freeSpace ? totalSamples : freeSpace;
	memcpy(buffer + bufferIndex, samples, toCopy * sizeof(float));
	bufferIndex += toCopy;
	printf("AFTER: %zu\n", bufferIndex);
}

int main(int argc, char *argv[]) {
	struct sigaction action;
	action.sa_handler = handleInterrupt;

	if(sigaction(SIGINT, &action, NULL)) {
		fprintf(stderr, "Failed to add SIGINT handler\n");
		return 1;
	}

	if(sigaction(SIGTERM, &action, NULL)) {
		fprintf(stderr, "Failed to add SIGTERM handler\n");
		return 1;
	}

	pwm_debugEnableLog(true);
	pwm_sysConnect(&argc, &argv);

	pwm_IO *in = pwm_ioCreateInput("To Out", true);
	// pwm_IO *in2 = pwm_ioCreateInput("To Out2 only", false);
	pwm_IO *out = pwm_ioCreateOutput("Out", false);
	// pwm_IO *out2 = pwm_ioCreateOutput("Out2", true);
	pwm_ioConnect(in, out);
	// pwm_ioConnect(in, out2);
	// pwm_ioConnect(in2, out2);

	pwm_ioSetConnectionFilter(in, out, doFilter2);

	// bool loud = false;
	while(pwm_sysIsRunning()) {
		// loud = !loud;
		// pwm_ioSetConnectionVolume(in, out, loud ? 1.0f : 0.0f);

		/*printf("In  ");
		for(int i = 0; i < pwm_ioGetLastVolume(in) * 100; i++) {
			printf("#");
		}
		printf("\n");

		printf("Out ");
		for(int i = 0; i < pwm_ioGetLastVolume(out) * 100; i++) {
			printf("#");
		}
		printf("\n");*/

		usleep(1000 * 1000);
	}

	pwm_sysDisconnect();

	return 0;
}
