/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Maciej Baczmanski, Jakub Mazur
 */

// TODO - common error codes for controller and storage (errors.h)
enum StorageErrorCodes {
	SUCCESS_STORAGE = 0
};

// TODO - make template scallable from KConfig
// (What should be included in template and what shouldn't)
struct Template {
	char name[CONFIG_TEMPLATE_NAME_SIZE];
	uint32_t speed;
};

// TODO - docstrings and order of functions

void get_templates(struct Template *templates);
int get_template_by_name(char *name, struct Template *result);
void set_template(struct Template new_template);
uint8_t get_template_size(void);
void remove_template_by_name(char *name);

void init_storage(void);
int factory_reset(void);

int get_current_template(struct Template *result);
void set_current_template(char *name);
