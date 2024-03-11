// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Maciej Baczmanski, Michal Kawiak, Jakub Mazur
#if defined(CONFIG_UART_SHELL_SUPPORT)

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include "driver.h"
#include "storage.h"
#include "return_codes.h"


enum ChannelNumber relevant_channel = CH0;

static int cmd_channel(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc == 1) {
		shell_fprintf(shell, SHELL_NORMAL, "relevant channel: %d\n", relevant_channel);
	} else if (argc == 2) {
		enum ChannelNumber new_channel = (enum ChannelNumber)strtol(argv[1], NULL, 10);

		if (new_channel >= CONFIG_SUPPORTED_CHANNEL_NUMBER) {
			shell_fprintf(shell, SHELL_NORMAL,
			"New channel %d higher than number of supported channels %d\n",
			new_channel, CONFIG_SUPPORTED_CHANNEL_NUMBER);
			return 0;
		}
		relevant_channel = new_channel;
		shell_fprintf(shell, SHELL_NORMAL, "channel set to: %d\n", relevant_channel);
	}
	return 0;
}

static int cmd_speed(const struct shell *shell, size_t argc, char *argv[])
{
	// TODO - add reset current template in this function
	int ret;
	uint32_t speed_mrpm;

	if (argc == 1) {
		ret = speed_get(relevant_channel, &speed_mrpm);
		if (ret == SUCCESS) {
			shell_fprintf(shell, SHELL_NORMAL, "speed: %d\n", speed_mrpm);

		} else if (ret == ERR_NOT_INITIALISED) {
			shell_fprintf(shell, SHELL_ERROR,
				"driver not initialised, could't get speed!\n");

		} else {
			shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
		}

		return 0;
	} else if (argc == 2) {
		speed_mrpm = (uint32_t)strtol(argv[1], NULL, 10);
		ret = target_speed_set(speed_mrpm, relevant_channel);

		if (ret == SUCCESS) {
			shell_fprintf(shell, SHELL_NORMAL, "speed set to: %d\n", speed_mrpm);

		} else if (ret == ERR_NOT_INITIALISED) {
			shell_fprintf(shell, SHELL_ERROR,
				"driver not initialised, could't set speed!\n");

		} else if (ret == ERR_DESIRED_VALUE_TO_HIGH) {
			shell_fprintf(shell, SHELL_ERROR,
				"Desired speed to high! Desired speed: %d; Max speed: %d\n",
				speed_mrpm, get_current_max_speed());

		} else if (ret == ERR_UNSUPPORTED_FUNCTION_IN_CURRENT_MODE) {
			shell_fprintf(shell, SHELL_ERROR,
				"Function unsupported in current mode!\n");

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
		if (get_motor_off_on(relevant_channel)) {
			shell_fprintf(shell, SHELL_NORMAL,
				      "Motor channel %d is turned on!\n", relevant_channel);

		} else {
			shell_fprintf(shell, SHELL_NORMAL,
				      "Motor channel %d is turned off!\n", relevant_channel);
		}

	} else if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			ret = motor_on(FORWARD, relevant_channel);

		} else if (strcmp(argv[1], "stop") == 0) {
			ret = motor_off(relevant_channel);

		} else {
			shell_fprintf(shell, SHELL_ERROR, "Unknown subcommand!\n");
			return 0;
		}

		if (ret != 0) {
			shell_fprintf(shell, SHELL_ERROR,
				"Couldn't change motor state! Error %d\n", ret);

		} else {
			shell_fprintf(shell, SHELL_NORMAL,
				      "Operation executed on channel %d!\n", relevant_channel);
		}

	} else if (argc == 3) {
		if (strcmp(argv[1], "start") == 0) {
			if (strcmp(argv[2], "fwd") == 0 ||
			    strcmp(argv[2], "forward") == 0 ||
			    strcmp(argv[2], "f") == 0) {
				ret = motor_on(FORWARD, relevant_channel);

			} else if (strcmp(argv[2], "bck") == 0 ||
				   strcmp(argv[2], "backward") == 0 ||
				   strcmp(argv[2], "b") == 0) {
				ret = motor_on(BACKWARD, relevant_channel);

			} else {
				shell_fprintf(shell, SHELL_ERROR,
					      "Unknown subcommand!, %s\n", argv[2]);
				return 0;
			}

			if (ret != 0) {
				shell_fprintf(shell, SHELL_ERROR,
					      "Couldn't change motor state! Error %d\n\n", ret);

			} else {
				shell_fprintf(shell, SHELL_NORMAL, "Operation executed!\n");
			}

		} else {
			shell_fprintf(shell, SHELL_ERROR, "Unknown subcommand!, %s\n", argv[1]);
		}
	}

	return 0;
}

static int cmd_position(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	uint32_t position;

	if (argc == 1) {
		ret = position_get(&position, relevant_channel);
		if (ret == SUCCESS) {
			shell_fprintf(shell, SHELL_NORMAL, "Position: %d\n", position);

		} else if (ret == ERR_NOT_INITIALISED) {
			shell_fprintf(shell, SHELL_ERROR,
				"Driver not initialised, could't get position!\n");

		} else {
			shell_fprintf(shell, SHELL_ERROR, "Other error, code: %d\n", ret);
		}

		return 0;
	} else if (argc == 2) {
		position = (uint32_t)strtol(argv[1], NULL, 10);
		ret = target_position_set(position, relevant_channel);
		if (ret == SUCCESS) {
			shell_fprintf(shell, SHELL_NORMAL, "Position set to: %d\n", position);

		} else if (ret == ERR_NOT_INITIALISED) {
			shell_fprintf(shell, SHELL_ERROR,
				"Driver not initialised, could't set position!\n");

		} else if (ret == ERR_DESIRED_VALUE_TO_HIGH) {
			shell_fprintf(shell, SHELL_ERROR,
				"Desired position to high! Desired: %d. Max: 360\n", position);

		} else if (ret == ERR_UNSUPPORTED_FUNCTION_IN_CURRENT_MODE) {
			shell_fprintf(shell, SHELL_ERROR,
			"Function unsupported in current mode!\n");

		} else {
			shell_fprintf(shell, SHELL_ERROR, "Other error, code: %d\n", ret);
		}
		// TODO - improve messages so that there is no chain of if elses -
		// eg. map: error code -> message?
	}
	return 0;
}

static int cmd_mode(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	enum ControlModes mode;

	if (argc == 1) {
		ret = mode_get(&mode);
		if (ret == SUCCESS) {
			char *mode_as_str = "";
			int r = get_control_mode_as_string(mode, &mode_as_str);

			if (r == SUCCESS) {
				shell_fprintf(shell, SHELL_NORMAL, "mode: %s\n", mode_as_str);

			} else {
				shell_fprintf(shell, SHELL_ERROR,
				"Conversion error, code: %d\n", ret);

			}
		} else if (ret == ERR_NOT_INITIALISED) {
			shell_fprintf(shell, SHELL_ERROR,
				"driver not initialised, could't get mode!\n");

		} else {
			shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
		}

		return 0;
	} else if (argc == 2) {
		ret = get_control_mode_from_string(argv[1], &mode);
		if (ret != SUCCESS) {
			shell_fprintf(shell, SHELL_ERROR,
				"Setting Control Mode crashed at conversion, errc: %d\n", ret);
		}

		ret = mode_set(mode);
		if (ret == SUCCESS) {
			shell_fprintf(shell, SHELL_NORMAL, "mode set to: %s\n", argv[1]);

		} else if (ret == ERR_NOT_INITIALISED) {
			shell_fprintf(shell, SHELL_ERROR,
				"driver not initialised, could't set mode!\n");

		} else {
			shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
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
					 "CalcSpeed: %u\n"
					 "pos target: %u\n"
					 "pos delta %d",
	get_cycles_count_DEBUG(),
	get_time_cycles_count_DEBUG(),
	get_ret_DEBUG(),
	get_calc_speed_DEBUG(),
	get_target_pos_DEBUG(),
	get_pos_d_DEBUG()
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
	int error = get_current_template(&current);

	if (error == SUCCESS) {
		shell_fprintf(shell, SHELL_INFO,
			      "Name: %-9s speed %d\n", current.name, current.speed);
	} else {
		shell_fprintf(shell, SHELL_WARNING,
			      "Error while checking active template: %d!\n", error);

	}

	return 0;
}

static int cmd_template_get(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t size = get_template_size();
	int error;

	if (argc == 1) {
		struct Template *tmp = (struct Template *)malloc(size * sizeof(struct Template));

		error = get_templates(tmp);
		if(error == ERR_EMPTY_TEMPLATE_LIST){
			shell_fprintf(shell, SHELL_WARNING, "Template list is empty!\n");
			return 0;
		} else if(error != SUCCESS){
			shell_fprintf(shell, SHELL_WARNING,
				      "Error while getting template list: %d!\n", error);
		}

		for (int i = 0; i < size; i++) {
			shell_fprintf(shell, SHELL_INFO,
				      "Name: %-9s speed %d\n", tmp[i].name, tmp[i].speed);
		}

		free(tmp);
	} else {
		struct Template res;
		uint8_t template_id;

		int error = get_template_and_id_by_name(argv[1], &res, &template_id); // TODO -NULL?
		if(error == SUCCESS) {
			shell_fprintf(shell, SHELL_INFO, "speed %d\n", res.speed);
		} else if(error == ERR_COULDNT_FIND_TEMPLATE) {
			shell_fprintf(shell, SHELL_WARNING, "Couldn't find this template!\n");
		} else if(error == ERR_EMPTY_TEMPLATE_LIST){
			shell_fprintf(shell, SHELL_WARNING, "Template list is empty!\n");
		} else {
			shell_fprintf(shell, SHELL_ERROR,
				      "Other error during template get! Error Code: %d\n", error);
		}
	}

	return 0;
}

static int cmd_template_apply(const struct shell *shell, size_t argc, char *argv[])
{
	struct Template res;
	uint8_t template_id;
	int ret;

	ret = get_template_and_id_by_name(argv[1], &res, &template_id);
	if(ret == ERR_COULDNT_FIND_TEMPLATE) {
		shell_fprintf(shell, SHELL_WARNING, "Couldn't find template!\n");
		return 0;
	} else if(ret != SUCCESS) {
		shell_fprintf(shell, SHELL_ERROR,
			      "Other error while searching for saved template: %d!\n", ret);
		return 0;
	}

	ret = target_speed_set(res.speed, relevant_channel);
	if (ret == SUCCESS) {
		shell_fprintf(shell, SHELL_NORMAL, "speed set to: %d on channel %d\n", res.speed, relevant_channel);

	} else if (ret == ERR_DESIRED_VALUE_TO_HIGH) {
		shell_fprintf(shell, SHELL_ERROR,
			"Desired template speed too high! Desired speed: %d; Max speed: %d\n",
			res.speed, get_current_max_speed());
		return 0;

	} else if (ret == ERR_UNSUPPORTED_FUNCTION_IN_CURRENT_MODE) {
		shell_fprintf(shell, SHELL_ERROR,
			"Function unsupported in current mode!\n");
		return 0;

	} else {
		shell_fprintf(shell, SHELL_ERROR, "other error, code: %d\n", ret);
		return 0;
	}

	ret = set_current_template(res.name);
	if(ret != SUCCESS){
		shell_fprintf(shell, SHELL_ERROR, "But couldn't update current template value! error %d\n", ret);
	}
	return 0;
}

static int cmd_template_set(const struct shell *shell, size_t argc, char *argv[])
{
	struct Template new_template;
	int ret;

	strcpy(new_template.name, argv[1]);
	new_template.speed = (uint32_t)strtol(argv[2], NULL, 10);
	ret = set_template(new_template);
	if(ret == SUCCESS){
		shell_fprintf(shell, SHELL_INFO, "Done, new template created\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Error while setting new template, code: %d\n", ret);
	}

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

SHELL_CMD_ARG_REGISTER(channel, NULL, "Get/set current relevant channel", cmd_channel, 1, 1);

SHELL_CMD_ARG_REGISTER(speed, NULL,
	"speed in milli RPM (one thousands of RPM)\nspeed to get speed\n speed <val> to set speed",
	cmd_speed, 1, 1);

SHELL_CMD_ARG_REGISTER(motor, NULL, "Start or stop motor\nstart <f b>\noff", cmd_off_on, 1, 2);
SHELL_CMD_ARG_REGISTER(pos, NULL, "get/set position in deca-degree", cmd_position, 1, 1);
SHELL_CMD_ARG_REGISTER(mode, NULL, "choose mode, pos/speed control. Default speed", cmd_mode, 1, 1);
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
SHELL_CMD_REGISTER(boot, NULL,
	"Enter bootloader mode, in order to flash new software via nRF connect programmer",
	cmd_boot);
#endif


#endif
