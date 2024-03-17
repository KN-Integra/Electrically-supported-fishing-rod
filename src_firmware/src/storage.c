// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024 Maciej Baczmanski, Jakub Mazur

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/sys/__assert.h>

#include "storage.h"

// TODO - correct all errno return var names to common, eg. errno_error or something like that
// TODO - correct all "our" return codes var names to common ret or something like that

#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define ALLOC_SIZE_ID 1
#define CURRENT_TEMPLATE_ID 2
#define TEMPLATES_START_ID 3

static struct nvs_fs fs;

static uint8_t alloc_size;

static char current_template[CONFIG_TEMPLATE_NAME_SIZE];

static bool initialized;

#pragma region ErrorManagement
static int errno_error_code;
static unsigned int errnor_error_line;

static void handle_errno_error(int error, unsigned int line_no)
{
	errno_error_code = error;
	errnor_error_line = line_no;
}
int get_errno_error_code(void)
{
	return errno_error_code;
}
unsigned int get_errno_error_line(void)
{
	return errnor_error_line;
}
#pragma endregion ErrorManagement

void init_storage(void)
{
	int rc = 0;
	struct flash_pages_info info;

	fs.flash_device = NVS_PARTITION_DEVICE;
	__ASSERT(device_is_ready(fs.flash_device),
		 "Flash device %s is not ready\n",
		 fs.flash_device->name);

	fs.offset = NVS_PARTITION_OFFSET;

	__ASSERT_EVAL((void)flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info),
			int error = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info),
			!error, "Unable to get page info");

	fs.sector_size = info.size;
	fs.sector_count = 3U;

	__ASSERT_EVAL((void)nvs_mount(&fs), int error = nvs_mount(&fs),
		      !error, "Flash Init failed");

	rc = nvs_read(&fs, ALLOC_SIZE_ID, &alloc_size, sizeof(uint8_t));
	if (rc <= 0) {
		alloc_size = 0;
		(void)nvs_write(&fs, ALLOC_SIZE_ID, &alloc_size, sizeof(uint8_t));
	}

	nvs_read(&fs, CURRENT_TEMPLATE_ID, current_template,
		 CONFIG_TEMPLATE_NAME_SIZE * sizeof(char));

	initialized = true;
}

uint8_t get_template_size(void)
{
	return alloc_size;
}

return_codes_t set_template(struct Template new_template)
{
	if (!initialized) {
		return ERR_NOT_INITIALISED;
	}

	struct Template tmp_template;
	uint8_t idx;
	return_codes_t error_get_template;
	int error_write;

	// Get template index from name.
	error_get_template = get_template_and_id_by_name(new_template.name, &tmp_template, &idx);
	if (error_get_template == SUCCESS) { // if index exists:
		error_write = nvs_write(&fs, idx + TEMPLATES_START_ID, // write new template
				&new_template, sizeof(struct Template));
		if (error_write < 0) {
			handle_errno_error(error_write, __LINE__);
			return ERR_ERROR_CODE_FROM_ERRNO;
		}
		if (error_write == 0) {
			return ERR_NOTHING_WRITTEN_DURING_NVS_WRITE;
		}

	} else if (error_get_template == ERR_COULDNT_FIND_TEMPLATE ||
		   error_get_template == ERR_EMPTY_TEMPLATE_LIST) {
		   // if new index needs to be created

		if (alloc_size >= 254) {
			return ERR_TOO_MANY_TEMPLATES_DEFINED;
			// TODO - define max amount of templates in kconfig with range 1- 254
		}

		error_write = nvs_write(&fs, alloc_size + TEMPLATES_START_ID,
				&new_template, sizeof(struct Template));

		if (error_write < 0) {
			handle_errno_error(error_write, __LINE__);
			return ERR_ERROR_CODE_FROM_ERRNO;
		}
		// TODO - sometimes error_write == 0 is expected when overwritin values?

		alloc_size++;
		error_write = nvs_write(&fs, ALLOC_SIZE_ID, &alloc_size, sizeof(uint8_t));

		if (error_write < 0) {
			handle_errno_error(error_write, __LINE__);
			return ERR_ERROR_CODE_FROM_ERRNO;
		}
		// TODO - sometimes error_write == 0 is expected when overwritin values?
	} else {
		return error_get_template;
	}

	return SUCCESS;
}

return_codes_t get_templates(struct Template *templates)
{
	if (!initialized) {
		return ERR_NOT_INITIALISED;
	}
	if (alloc_size == 0) {
		return ERR_EMPTY_TEMPLATE_LIST;
	}

	int error;

	for (int i = 0; i < alloc_size; i++) {
		error = nvs_read(&fs, i + TEMPLATES_START_ID,
			       &(templates[i]), sizeof(struct Template));

		if (error < 0) {
			handle_errno_error(error, __LINE__);
			return ERR_ERROR_CODE_FROM_ERRNO;
		}
		if (error == 0) {
			return ERR_NOTHING_READ_DURING_NVS_READ;
		}
	}
	return SUCCESS;
}

return_codes_t get_template_and_id_by_name(char *name, struct Template *result, uint8_t *out_id)
{
	if (!initialized) {
		return ERR_NOT_INITIALISED;
	}
	if (alloc_size == 0) {
		return ERR_EMPTY_TEMPLATE_LIST;
	}

	int error;

	for (int i = 0; i < alloc_size; i++) {
		error = nvs_read(&fs, i + TEMPLATES_START_ID, result, sizeof(struct Template));
		if (strcmp(name, result->name) == 0) {
			if (error < 0) {
				handle_errno_error(error, __LINE__);
				return ERR_ERROR_CODE_FROM_ERRNO;
			}
			if (error == 0) {
				return ERR_NOTHING_READ_DURING_NVS_READ;
			}

			*out_id = i;
			return SUCCESS;
		}
	}

	return ERR_COULDNT_FIND_TEMPLATE;
}

return_codes_t get_current_template(struct Template *result)
{
	if (!initialized) {
		return ERR_NOT_INITIALISED;
	}
	//TODO - what if speed was set outside of template?
	//(apply template -> set speed -> get current template)
	uint8_t idx; // TODO - turn it to NULL?
	// TODO - additional error check if we are outside of template.
	return get_template_and_id_by_name(current_template, result, &idx);
}

return_codes_t set_current_template(char *name)
{
	int error;

	if (!initialized) {
		return ERR_NOT_INITIALISED;
	}

	strcpy(current_template, name);
	// TODO - also check, if there is any current template applied
	// if not, there is nothing to set, so return error
	error = nvs_write(&fs, CURRENT_TEMPLATE_ID, current_template,
		  CONFIG_TEMPLATE_NAME_SIZE * sizeof(char));

	if (error < 0) {
		handle_errno_error(error, __LINE__);
		return ERR_ERROR_CODE_FROM_ERRNO;
	}
	if (error == 0) {
		return ERR_NOTHING_WRITTEN_DURING_NVS_WRITE;
	}

	return SUCCESS;
}

return_codes_t remove_template_by_name(char *name)
{
	struct Template template_to_remove;
	uint8_t idx;
	return_codes_t ret;

	if (!initialized) {
		return ERR_NOT_INITIALISED;
	}
	if (alloc_size == 0) {
		return ERR_EMPTY_TEMPLATE_LIST;
	}

	ret = get_template_and_id_by_name(name, &template_to_remove, &idx);
	if (ret == SUCCESS) {
		if (idx < alloc_size - 1) {
			struct Template last;
			int error_errno;

			error_errno = nvs_read(&fs, alloc_size + TEMPLATES_START_ID - 1,
						   &last, sizeof(struct Template));
			if (ret < 0) {
				handle_errno_error(error_errno, __LINE__);
				return ERR_ERROR_CODE_FROM_ERRNO;
			}
			if (ret == 0) {
				return ERR_NOTHING_READ_DURING_NVS_READ;
			}

			error_errno = nvs_write(&fs, idx + TEMPLATES_START_ID,
							&last, sizeof(struct Template));
			if (ret < 0) {
				handle_errno_error(error_errno, __LINE__);
				return ERR_ERROR_CODE_FROM_ERRNO;
			}
			if (ret == 0) {
				return ERR_NOTHING_WRITTEN_DURING_NVS_WRITE;
			}
		}
		alloc_size--;
		(void)nvs_write(&fs, ALLOC_SIZE_ID, &alloc_size, sizeof(uint8_t));
	} else {
		return ret;
	}

	return SUCCESS;

	//TODO - also remove current template if applicable (current template is one being removed)
}

return_codes_t factory_reset(void)
{
	if (alloc_size == 0) {
		return ERR_EMPTY_TEMPLATE_LIST;
	}

	int error;
	alloc_size = 0;
	current_template[0] = '\0';
	error = nvs_write(&fs, ALLOC_SIZE_ID, &alloc_size, sizeof(uint8_t));
	if (error < 0) {
		handle_errno_error(error, __LINE__);
		return ERR_ERROR_CODE_FROM_ERRNO;
	}
	if (error == 0) {
		return ERR_NOTHING_WRITTEN_DURING_NVS_WRITE;
	}

	error = nvs_write(&fs, CURRENT_TEMPLATE_ID, current_template,
		  CONFIG_TEMPLATE_NAME_SIZE * sizeof(char));

	if (error < 0) {
		handle_errno_error(error, __LINE__);
		return ERR_ERROR_CODE_FROM_ERRNO;
	}
	if (error == 0) {
		return ERR_NOTHING_WRITTEN_DURING_NVS_WRITE;
	}

	return SUCCESS;
}
