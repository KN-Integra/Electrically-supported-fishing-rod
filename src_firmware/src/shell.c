// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Maciej Baczmanski, Michal Kawiak, Jakub Mazur

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include "driver.h"
#include "storage.h"


static int cmd_init(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	ret = init_pwm_motor_driver(67000u); // TODO - change to argument
	if (ret) {
		shell_fprintf(shell, SHELL_ERROR, "usuccesful driver initialisation, errc: %d\n", ret);
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "initialisation succesful!\n");
	}

	return 0;
}

static int cmd_speed(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	uint32_t speed_mrpm;

	if (argc == 1) {
		ret = speed_get(&speed_mrpm);
		if (ret == SUCCESS) {
			shell_fprintf(shell, SHELL_NORMAL, "speed: %d\n", speed_mrpm);
		} else if (ret == NOT_INITIALISED) {
			shell_fprintf(shell, SHELL_ERROR, "driver not initialised, could't get speed!\n");
		} else {
			shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
		}

		return 0;
	} else if (argc == 2) {
		speed_mrpm = (uint32_t)strtol(argv[1], NULL, 10);
		ret = target_speed_set(speed_mrpm);
		if (ret == SUCCESS) {
			shell_fprintf(shell, SHELL_NORMAL, "speed set to: %d\n", speed_mrpm);
		} else if (ret == NOT_INITIALISED) {
			shell_fprintf(shell, SHELL_ERROR, "driver not initialised, could't set speed!\n");
		} else if (ret == DESIRED_SPEED_TO_HIGH) {
			shell_fprintf(shell, SHELL_ERROR, "Desired speed to high! Desired speed: %d; Max speed: %d\n", speed_mrpm, get_current_max_speed());
		} else {
			shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
		}
	}
	return 0;
}

static int cmd_off_on(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	if (argc == 1) {
		if (get_motor_off_on()) {
			shell_fprintf(shell, SHELL_NORMAL, "Motor is turned on!\n");
		} else {
			shell_fprintf(shell, SHELL_NORMAL, "Motor is turned off!\n");
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			ret = motor_on(FORWARD);
		} else if (strcmp(argv[1], "stop") == 0) {
			ret = motor_off();
		} else {
			shell_fprintf(shell, SHELL_ERROR, "Unknown subcommand!\n");
			return 0;
		}

		if (ret != 0) {
			shell_fprintf(shell, SHELL_ERROR, "Couldn't change motor state! Error %d\n", ret);
		} else {
			shell_fprintf(shell, SHELL_NORMAL, "Operation executed!\n");
		}

	} else if (argc == 3) {
		if (strcmp(argv[1], "start") == 0) {
			if (strcmp(argv[2], "fwd") == 0 || strcmp(argv[2], "forward") == 0 || strcmp(argv[2], "f") == 0) {
				ret = motor_on(FORWARD);
			} else if (strcmp(argv[2], "bck") == 0 || strcmp(argv[2], "backward") == 0 || strcmp(argv[2], "b") == 0) {
				ret = motor_on(BACKWARD);
			} else {
				shell_fprintf(shell, SHELL_ERROR, "Unknown subcommand!, %s\n", argv[2]);
				return 0;
			}

			if (ret != 0) {
				shell_fprintf(shell, SHELL_ERROR, "Couldn't change motor state! Error %d\n\n", ret);
			} else {
				shell_fprintf(shell, SHELL_NORMAL, "Operation executed!\n");
			}

		} else {
			shell_fprintf(shell, SHELL_ERROR, "Unknown subcommand!, %s\n", argv[1]);
		}
	}

	return 0;
}

#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
static int cmd_boot(const struct shell *shell, size_t argc, char *argv[])
{
	enter_boot();
	return 0;
}
#endif

static int cmd_debug(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_INFO, "Current cycles count: %" PRIu64  "\n"
					 "Time cycles count: %" PRIu64  "\n"
					 "Ret: %d\n"
					 "CalcSpeed: %u\n",
	get_cycles_count_DEBUG(),
	get_time_cycles_count_DEBUG(),
	get_ret_DEBUG(),
	get_calc_speed_DEBUG()
	);
	return 0;
}

static int cmd_drv_version(const struct shell *shell, size_t argc, char *argv[])
{
	const struct DriverVersion tmp_ver = get_driver_version();

	shell_fprintf(shell, SHELL_ERROR,
		      "Software version: %d.%d\n", tmp_ver.major, tmp_ver.minor);
	return 0;
}

static int cmd_template_active(const struct shell *shell, size_t argc, char *argv[])
{
	struct Template current;
	int idx = get_current_template(&current);

	if (idx >= 0) {
		shell_fprintf(shell, SHELL_INFO,
			      "Name: %-9s speed %d\n", current.name, current.speed);
	} else {
		shell_fprintf(shell, SHELL_WARNING, "Template not initialized!\n");

	}

	return 0;
}

static int cmd_template_get(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t size = get_template_size();

	if (argc == 1) {
		struct Template *tmp = (struct Template *)malloc(size * sizeof(struct Template));

		get_templates(tmp);
		for (int i = 0; i < size; i++) {
			shell_fprintf(shell, SHELL_INFO,
				      "Name: %-9s speed %d\n", tmp[i].name, tmp[i].speed);
		}

		free(tmp);
	} else {
		struct Template res;

		get_template_by_name(argv[1], &res);
		shell_fprintf(shell, SHELL_INFO, "speed %d\n", res.speed);
	}

	return 0;
}

static int cmd_template_apply(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t size = get_template_size();
	struct Template *tmp = (struct Template *)malloc(size * sizeof(struct Template));
	struct Template res;

	get_templates(tmp);
	get_template_by_name(argv[1], &res);

	target_speed_set(res.speed);
	set_current_template(res.name);

	shell_fprintf(shell, SHELL_ERROR, "Speed template:\n");

	free(tmp);
	return 0;
}

static int cmd_template_set(const struct shell *shell, size_t argc, char *argv[])
{
	struct Template new_template;

	strcpy(new_template.name, argv[1]);
	new_template.speed = (uint32_t)strtol(argv[2], NULL, 10);
	set_template(new_template);
	shell_fprintf(shell, SHELL_INFO, "Done\n");

	return 0;
}

static int cmd_template_clear(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc == 1) {
		int exit_command = factory_reset();

		shell_fprintf(shell, SHELL_INFO,
			      "Template clear exited with command: %d\n", exit_command);
	} else {
		remove_template_by_name(argv[1]);
	}
	return 0;
}

static int cmd_template_size(const struct shell *shell, size_t argc, char *argv[])
{
	int size = get_template_size();

	shell_fprintf(shell, SHELL_ERROR, "Current size of template buffer: %d\n", size);
	return 0;
}


SHELL_CMD_REGISTER(init, NULL, "Initialise PWM motors and GPIOs\ninit to use default values\ninit with args - TODO", cmd_init);
SHELL_CMD_ARG_REGISTER(speed, NULL, "speed in milli RPM (one thousands of RPM)\nspeed to get speed\nspeed <value> to set speed", cmd_speed, 1, 1);
SHELL_CMD_ARG_REGISTER(motor, NULL, "Start or stop motor\nstart <f b>\noff", cmd_off_on, 1, 2);
SHELL_CMD_REGISTER(debug, NULL, "get debug info", cmd_debug);
SHELL_CMD_REGISTER(drv_version, NULL, "get motor driver version", cmd_drv_version);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_template,
	SHELL_CMD_ARG(active, NULL, "Active template", cmd_template_active, 1, 0),
	SHELL_CMD_ARG(get, NULL, "get <optional name>", cmd_template_get, 1, 1),
	SHELL_CMD_ARG(apply,   NULL, "apply name", cmd_template_apply, 2, 0),
	SHELL_CMD_ARG(set,   NULL, "set name speed", cmd_template_set, 3, 0),
	SHELL_CMD_ARG(clear,   NULL, "clear template by name. If none given clears all templates.",
				  cmd_template_clear, 1, 1),
	SHELL_CMD_ARG(size,   NULL, "get amount of templates", cmd_template_size, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(template, &sub_template, "get/apply/create speed template", NULL);


#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
SHELL_CMD_REGISTER(boot, NULL, "Enter bootloader mode, in order to flash new software via nRF connect programmer", cmd_boot);
#endif
