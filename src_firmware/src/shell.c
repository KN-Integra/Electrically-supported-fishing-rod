#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include "driver.h"

static int cmd_init(const struct shell *shell, size_t argc, char *argv[]){
        int ret;
        uint32_t max_speed_mrpm = 67000u;
        ControlModes new_speed_mode = SPEED;
        uint32_t starting_position = 0u;
        if(argc >= 2){ //first arg is ma speed
                max_speed_mrpm = (uint32_t)strtol(argv[1], NULL, 10);
        } 
        if(argc >= 3){ // second arg is a control mode
                ret = get_control_mode_from_string(argv[2], &new_speed_mode);
                if(ret != SUCCESS){
                        shell_fprintf(shell, SHELL_ERROR, "Init crashed at Control Mode, errc: %d\n",ret);
                }
        }
        if(argc >= 4){// third arg is starting pos
                starting_position = (uint32_t)strtol(argv[3], NULL, 10);
        }

        ret = init_pwm_motor_driver(max_speed_mrpm, new_speed_mode, starting_position);
        if(ret){
                shell_fprintf(shell, SHELL_ERROR, "usuccesful driver initialisation, errc: %d\n",ret);
        }else{
                shell_fprintf(shell, SHELL_NORMAL, "initialisation succesful!\n");
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
                        shell_fprintf(shell, SHELL_NORMAL, "speed: %d\n", speed_mrpm);
                } else if(ret == NOT_INITIALISED){
                        shell_fprintf(shell, SHELL_ERROR, "driver not initialised, could't get speed!\n");
                } else{
                      shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
                }

                return 0;
        } else if(argc == 2){
                speed_mrpm = (uint32_t)strtol(argv[1], NULL, 10);
                ret = target_speed_set(speed_mrpm);
                if(ret == SUCCESS){
                        shell_fprintf(shell, SHELL_NORMAL, "speed set to: %d\n", speed_mrpm);
                } else if(ret == NOT_INITIALISED){
                        shell_fprintf(shell, SHELL_ERROR, "driver not initialised, could't set speed!\n");
                } else if(ret == DESIRED_VALUE_TO_HIGH){
                        shell_fprintf(shell, SHELL_ERROR, "Desired speed to high! Desired speed: %d; Max speed: %d\n", speed_mrpm, get_current_max_speed());
                } else{
                      shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
                }
        }
        return 0;
}


static int cmd_position(const struct shell *shell, size_t argc, char *argv[]){
        int ret;
        uint32_t position;
        if(argc == 1)
        {
                ret = position_get(&position);
                if(ret == SUCCESS){
                        shell_fprintf(shell, SHELL_NORMAL, "speed: %d\n", position);
                } else if(ret == NOT_INITIALISED){
                        shell_fprintf(shell, SHELL_ERROR, "driver not initialised, could't get position!\n");
                } else{
                      shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
                }

                return 0;
        } else if(argc == 2){
                position = (uint32_t)strtol(argv[1], NULL, 10);
                ret = target_position_set(position);
                if(ret == SUCCESS){
                        shell_fprintf(shell, SHELL_NORMAL, "speed set to: %d\n", position);
                } else if(ret == NOT_INITIALISED){
                        shell_fprintf(shell, SHELL_ERROR, "driver not initialised, could't set position!\n");
                } else if(ret == DESIRED_VALUE_TO_HIGH){
                        shell_fprintf(shell, SHELL_ERROR, "Desired position to high! Desired position: %d; Max position: 360 degree\n", position);
                } else{
                      shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
                }
        }
        return 0;
}

static int cmd_mode(const struct shell *shell, size_t argc, char *argv[]){
        int ret;
        ControlModes mode;
        if(argc == 1)
        {
                ret = mode_get(&mode);
                if(ret == SUCCESS){
                        shell_fprintf(shell, SHELL_NORMAL, "mode: %d\n", mode);
                } else if(ret == NOT_INITIALISED){
                        shell_fprintf(shell, SHELL_ERROR, "driver not initialised, could't get mode!\n");
                } else{
                      shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
                }

                return 0;
        } else if(argc == 2){
                ret = get_control_mode_from_string(argv[1], &mode);
                if(ret != SUCCESS){
                        shell_fprintf(shell, SHELL_ERROR, "Setting Control Mode crashed at conversion, errc: %d\n",ret);
                }

                ret = mode_set(mode);
                if(ret == SUCCESS){
                        shell_fprintf(shell, SHELL_NORMAL, "mode set to: %s\n", argv[1]);
                } else if(ret == NOT_INITIALISED){
                        shell_fprintf(shell, SHELL_ERROR, "driver not initialised, could't set mode!\n");
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

static int cmd_debug(const struct shell *shell, size_t argc, char *argv[]){
        uint32_t current_p;
        int ret = position_get(&current_p);

        shell_fprintf(shell, SHELL_INFO, "Current cycles count: %" PRIu64  "\n"
                                         "Time cycles count: %" PRIu64  "\n"
                                         "Ret: %d\n"
                                         "Pos: %d %d\n",
        get_cycles_count_DEBUG(),
        get_time_cycles_count_DEBUG(),
        get_ret_DEBUG(),
        current_p, ret
        );
        return 0;
}

static int cmd_drv_version(const struct shell *shell, size_t argc, char *argv[]){
        shell_fprintf(shell, SHELL_ERROR, "Software version: %s \n", get_driver_version());
        return 0;
}


SHELL_CMD_ARG_REGISTER(init, NULL, "Initialise PWM motors and GPIOs\ninit to use default values\ninit with args - TODO", cmd_init, 1, 3);
SHELL_CMD_ARG_REGISTER(speed, NULL, "speed in milli RPM (one thousands of RPM)\nspeed to get speed\nspeed <value> to set speed", cmd_speed, 1, 1);
SHELL_CMD_ARG_REGISTER(pos, NULL, "----------------------", cmd_position, 1, 1);
SHELL_CMD_ARG_REGISTER(mode, NULL, "----------------------", cmd_mode, 1, 1);
SHELL_CMD_REGISTER(boot, NULL, "Enter bootloader mode, in order to flash new software via nRF connect programmer", cmd_boot);
SHELL_CMD_REGISTER(debug, NULL, "get debug info", cmd_debug);
SHELL_CMD_REGISTER(drv_version, NULL, "get motor driver version", cmd_drv_version);
