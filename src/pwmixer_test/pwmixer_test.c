#include <pwmixer.h>

int main(int argc, char *argv[]) {
	pwm_sysConnect(argc, argv);

	pwm_Input *in = pwm_ioCreateInput("To Out + Out2", false);
	pwm_Input *in2 = pwm_ioCreateInput("To Out2 only", true);
	pwm_Output *out = pwm_ioCreateOutput("Out", true);
	pwm_Output *out2 = pwm_ioCreateOutput("Out2", false);
	pwm_ioConnect(in, out);
	pwm_ioConnect(in, out2);
	pwm_ioConnect(in2, out2);

	pwm_sysRun();
	pwm_sysDisconnect();
}