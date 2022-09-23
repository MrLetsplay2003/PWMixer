#include <pwmixer.h>

#include <stdio.h>
#include <signal.h>

static void handleInterrupt(int sig) {
	printf("Got interrupt\n");
	if(pwm_sysIsRunning()) pwm_sysDisconnect();
}

int main(int argc, char *argv[]) {
	if(signal(SIGINT, handleInterrupt) == SIG_ERR) {
		printf("Failed to add SIGINT handler\n");
		return 1;
	}

	if(signal(SIGTERM, handleInterrupt) == SIG_ERR) {
		printf("Failed to add SIGTERM handler\n");
		return 1;
	}

	pwm_sysConnect(&argc, &argv);

	pwm_IO *in = pwm_ioCreateInput("To Out + Out2", false);
	pwm_IO *in2 = pwm_ioCreateInput("To Out2 only", true);
	pwm_IO *out = pwm_ioCreateOutput("Out", true);
	pwm_IO *out2 = pwm_ioCreateOutput("Out2", false);
	pwm_ioConnect(in, out);
	pwm_ioConnect(in, out2);
	pwm_ioConnect(in2, out2);

	pwm_ioDestroy(in);
	pwm_ioDestroy(in2);
	pwm_ioDestroy(out);
	pwm_ioDestroy(out2);

	//while(pwm_sysIsRunning());
	pwm_sysDisconnect();

	//printf("Run #2\n");
	//pwm_sysConnect(argc, argv);
	//while(pwm_sysIsRunning());

	return 0;
}