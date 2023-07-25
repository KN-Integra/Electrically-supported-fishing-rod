#include<zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include "driver.h"

static int cmd_init(const struct shell *shell, size_t argc, char *argv[]){
        int ret;
        ret = init_pwm_motor_driver(67000u); // TODO - change to argument
        if(ret){
                shell_fprintf(shell, SHELL_ERROR, "usuccesful driver initialisation, errc: %d\n",ret);
        }else{
                shell_fprintf(shell, SHELL_ERROR, "initialisation succesful!\n");
        }

        return 0;
}

static int cmd_speed(const struct shell *shell, size_t argc, char *argv[]){
        int ret;
        uint32_t speed_mrpm;
        if(argc == 1)
        {
                ret = speed_get(&speed_mrpm);
                if(ret == SUCCESS){
                        shell_fprintf(shell, SHELL_ERROR, "speed: %d\n", speed_mrpm);
                } else if(ret == NOT_INITIALISED){
                        shell_fprintf(shell, SHELL_ERROR, "driver not initialised, could't get speed!\n");
                } else{
                      shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
                }

                return 0;
        } else if(argc == 2){
                speed_mrpm = (uint32_t)strtol(argv[1], NULL, 10);
                ret = speed_set(speed_mrpm);
                if(ret == SUCCESS){
                        shell_fprintf(shell, SHELL_ERROR, "speed set to: %d\n", speed_mrpm);
                } else if(ret == NOT_INITIALISED){
                        shell_fprintf(shell, SHELL_ERROR, "driver not initialised, could't set speed!\n");
                } else if(ret == DESIRED_SPEED_TO_HIGH){
                        shell_fprintf(shell, SHELL_ERROR, "Desired speed to high! Desired speed: %d; Max speed: %d\n", speed_mrpm, get_current_max_speed());
                } else{
                      shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
                }
        }
        return 0;
}



static int cmd_boot(const struct shell *shell, size_t argc, char *argv[]){
        enter_boot();
        return 0;
}


SHELL_CMD_REGISTER(init, NULL, "Initialise PWM motors and GPIOs\ninit to use default values\ninit with args - TODO", cmd_init);
SHELL_CMD_ARG_REGISTER(speed, NULL, "speed in milli RPM (one thousands of RPM)\nspeed to get speed\nspeed <value> to set speed", cmd_speed, 1, 1);
SHELL_CMD_REGISTER(boot, NULL, "Enter bootloader mode, in order to flash new software via nRF connect programmer", cmd_boot);
