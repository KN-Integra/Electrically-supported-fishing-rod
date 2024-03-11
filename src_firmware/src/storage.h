/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Maciej Baczmanski, Jakub Mazur
 */

// TODO - make template scallable from KConfig
// (What should be included in template and what shouldn't)
struct Template {
	char name[CONFIG_TEMPLATE_NAME_SIZE];
	uint32_t speed;
};
// TODO - new feature, kconfig possibility for continuus template saving (whatever is currently
// in template), so that last template will load on startup (will it kill NVM?)
// TODO - another new, simplier feature - quick command that saves current setup to template.
// TODO - docstrings and order of functions

int get_templates(struct Template *templates);
int get_template_and_id_by_name(char *name, struct Template *result, uint8_t *out_id);
int set_template(struct Template new_template);
uint8_t get_template_size(void);
int remove_template_by_name(char *name);

void init_storage(void);
int factory_reset(void);

int get_current_template(struct Template *result);
int set_current_template(char *name);

int get_errno_error_code();
unsigned int get_errno_error_line();
