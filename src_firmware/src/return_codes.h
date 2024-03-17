/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Maciej Baczmanski, Michal Kawiak, Jakub Mazur
 */
#pragma once

typedef enum {
	SUCCESS = 0,

	// Minor Errors (range 1-15) - Can happen during normal operation
	ERR_UNSUPPORTED_FUNCTION_IN_CURRENT_MODE = 1,
	ERR_DESIRED_VALUE_TO_HIGH = 2,
	ERR_TOO_MANY_TEMPLATES_DEFINED = 3,
	ERR_COULDNT_FIND_TEMPLATE = 4,
	ERR_EMPTY_TEMPLATE_LIST = 5,

	// Critical errors (16+) - Are result of serious hardware issue
	ERR_NOT_INITIALISED = 16,

	ERR_UNABLE_TO_SET_PWM = 17,
	ERR_UNABLE_TO_SET_GPIO = 18,

	ERR_VALUE_CONVERSION_ERROR = 19,

	ERR_ERROR_CODE_FROM_ERRNO = 20,
	ERR_NOTHING_WRITTEN_DURING_NVS_WRITE = 21,
	ERR_NOTHING_READ_DURING_NVS_READ = 22,
} return_codes_t;
