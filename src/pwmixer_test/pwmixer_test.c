#include <pwmixer.h>

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

static void handleInterrupt(int sig) {
	pwm_sysDisconnect();
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
	//pwm_IO *in2 = pwm_ioCreateInput("To Out2 only", false);
	pwm_IO *out = pwm_ioCreateOutput("Out", false);
	//pwm_IO *out2 = pwm_ioCreateOutput("Out2", true);
	pwm_ioConnect(in, out);
	//pwm_ioConnect(in, out2);
	//pwm_ioConnect(in2, out2);

	bool loud = false;
	while(pwm_sysIsRunning()) {
		loud = !loud;
		//pwm_ioSetConnectionVolume(in, out, loud ? 1.0f : 0.0f);
		usleep(1000 * 1000);
	}

	pwm_sysDisconnect();

	return 0;
}