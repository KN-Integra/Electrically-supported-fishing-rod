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

#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define ALLOC_SIZE_ID 1
#define CURRENT_TEMPLATE_ID 2
#define TEMPLATES_START_ID 3

static struct nvs_fs fs;

static uint8_t alloc_size;

static char current_template[CONFIG_TEMPLATE_NAME_SIZE];


void init_storage(void)
{
	static bool initialized;

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

void set_template(struct Template new_template)
{
	struct Template tmp_template;
	int idx;

	idx = get_template_by_name(new_template.name, &tmp_template);
	if (idx >= 0) {
		(void)nvs_write(&fs, idx + TEMPLATES_START_ID,
				&new_template, sizeof(struct Template));

	} else {
		(void)nvs_write(&fs, alloc_size + TEMPLATES_START_ID,
				&new_template, sizeof(struct Template));

		alloc_size++;
		(void)nvs_write(&fs, ALLOC_SIZE_ID, &alloc_size, sizeof(uint8_t));
	}
}

void get_templates(struct Template *templates)
{
	for (int i = 0; i < alloc_size; i++) {
		(void)nvs_read(&fs, i + TEMPLATES_START_ID,
			       &(templates[i]), sizeof(struct Template));
	}
}

int get_template_by_name(char *name, struct Template *result)
{ // TODO return error code, id as pointer
	for (int i = 0; i < alloc_size; i++) {
		(void)nvs_read(&fs, i + TEMPLATES_START_ID, result, sizeof(struct Template));
		if (strcmp(name, result->name) == 0) {
			return i;
		}
	}
	return -1;
}

int get_current_template(struct Template *result)
{
	//TODO - what if speed was set outside of template?
	//(apply template -> set speed -> get current template)
	// TODO - also pass properly results from get_template_by_name comment
	return get_template_by_name(current_template, result);
}

void set_current_template(char *name)
{
	strcpy(current_template, name);
	nvs_write(&fs, CURRENT_TEMPLATE_ID, current_template,
		  CONFIG_TEMPLATE_NAME_SIZE * sizeof(char));
}

void remove_template_by_name(char *name)
{
	struct Template template_to_remove;
	int idx;

	if (alloc_size == 0) {
		return;
	}
	idx = get_template_by_name(name, &template_to_remove);
	if (idx >= 0) {
		if (idx < alloc_size - 1) {
			struct Template last;
			(void)nvs_read(&fs, alloc_size + TEMPLATES_START_ID - 1,
						   &last, sizeof(struct Template));

			(void)nvs_write(&fs, idx + TEMPLATES_START_ID,
							&last, sizeof(struct Template));
		}
		alloc_size--;
		(void)nvs_write(&fs, ALLOC_SIZE_ID, &alloc_size, sizeof(uint8_t));
	}

	//TODO - also remove current template if applicable (current template is one being removed)
}

int factory_reset(void)
{
	alloc_size = 0;
	current_template[0] = '\0';
	(void)nvs_write(&fs, ALLOC_SIZE_ID, &alloc_size, sizeof(uint8_t));
	nvs_write(&fs, CURRENT_TEMPLATE_ID, current_template,
		  CONFIG_TEMPLATE_NAME_SIZE * sizeof(char));

	return SUCCESS_STORAGE;
}
