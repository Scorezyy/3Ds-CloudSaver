#ifndef CLOUDSAVER_INPUT_H
#define CLOUDSAVER_INPUT_H

#include "common.h"

bool input_prompt_device_name(char *out, size_t max_len);
bool input_keyboard(const char *hint, const char *initial, char *out, size_t max_len);

#endif
