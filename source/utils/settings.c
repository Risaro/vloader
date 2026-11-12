/*
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include "settings.h"

#include "utils/datapath.h"

static const char* config_file_path(void) {
    static char path[320];
    if (!path[0])
        snprintf(path, sizeof(path), "%sconfig.txt", data_path());
    return path;
}

int  setting_sampleSetting;
bool setting_sampleSetting2;

void settings_reset() {
    setting_sampleSetting  = 1;
    setting_sampleSetting2 = true;
}

void settings_load() {
    settings_reset();

    char buffer[30];
    int value;

    FILE *config = fopen(config_file_path(), "r");

    if (config) {
        while (EOF != fscanf(config, "%[^ ] %d\n", buffer, &value)) {
            if 		(strcmp("setting_sampleSetting", buffer) == 0) 	setting_sampleSetting  = (int)value;
            else if (strcmp("setting_sampleSetting2", buffer) == 0) setting_sampleSetting2 = (bool)value;
        }
        fclose(config);
    }
}

void settings_save() {
    FILE *config = fopen(config_file_path(), "w+");

    if (config) {
        fprintf(config, "%s %d\n", "setting_sampleSetting", (int)(setting_sampleSetting));
        fprintf(config, "%s %d\n", "setting_sampleSetting2", (int)(setting_sampleSetting2));
        fclose(config);
    }
}
