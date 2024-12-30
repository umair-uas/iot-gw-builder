#include "file_storage.h"

static const char *TAG = "FILE_STORAGE";

void init_storage() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS initialization failed (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS initialized successfully");
    }
}

void save_data_to_file(const char *filename, const char *data) {
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "/spiffs/%s", filename);

    FILE *f = fopen(filepath, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        return;
    }

    fprintf(f, "%s\n", data);
    fclose(f);
    ESP_LOGI(TAG, "Data saved to file: %s", filepath);
}

void read_data_from_file(const char *filename) {
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "/spiffs/%s", filename);

    FILE *f = fopen(filepath, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);  // Print or process the data
    }
    fclose(f);
}

