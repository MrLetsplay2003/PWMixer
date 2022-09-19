#include <stdio.h>

#include "pwmixer.h"

pwm_Data *pwm_data;

int main(int argc, char *argv[]) {
	pwm_sysConnect(argc, argv);

	pwm_Input *in = pwm_ioCreateInput("To out + out2");
	pwm_Input *in2 = pwm_ioCreateInput("To out2 only");
	pwm_Output *out = pwm_ioCreateOutput("Out");
	//pwm_Output *out2 = pwm_ioCreateOutput("Out2");
	pwm_ioConnect(in, out);
	//pwm_ioConnect(in, out2);
	pwm_ioConnect(in2, out);

	pwm_sysRun();
	pwm_sysDisconnect();
}

void ayyo() {
	printf("AYYO\n");
}

void pwm_sysConnect(int argc, char **argv) {
	pw_init(&argc, &argv);

	pwm_data = calloc(1, sizeof(pwm_Data));
	pwm_data->mainLoop = pw_main_loop_new(NULL);
	pwm_data->context = pw_context_new(pw_main_loop_get_loop(pwm_data->mainLoop), NULL, 0);
	pwm_data->core = pw_context_connect(pwm_data->context, NULL, 0);

	printf("Connected to PipeWire\n");
	pw_loop_add_signal(pw_main_loop_get_loop(pwm_data->mainLoop), SIGINT, pwm_sysDisconnect, NULL);
	pw_loop_add_signal(pw_main_loop_get_loop(pwm_data->mainLoop), SIGTERM, pwm_sysDisconnect, NULL);
	//pw_loop_add_signal(pw_main_loop_get_loop(data->mainLoop), PWM_CREATE_INPUT, ayyo, NULL);
}

void pwm_sysRun() {
	printf("Running\n");
	pw_main_loop_run(pwm_data->mainLoop);
}

void pwm_sysDisconnect() {
	printf("PWMixer exiting\n");
	pw_core_disconnect(pwm_data->core);
	pw_context_destroy(pwm_data->context);
	pw_main_loop_destroy(pwm_data->mainLoop);
	pw_deinit();
}