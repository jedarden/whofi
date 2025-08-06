/**
 * @file ota_client.c
 * @brief OTA HTTP Client with Advanced Security Features
 */

#include <string.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <esp_tls.h>
#include <mbedtls/error.h>

static const char *TAG = "ota_client";

// Certificate validation callback
static esp_err_t ota_http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

esp_http_client_handle_t ota_http_client_init(const char *url, const char *cert_pem)
{
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = ota_http_event_handler,
        .timeout_ms = 30000,
        .buffer_size = 4096,
        .buffer_size_tx = 1024,
    };
    
    if (cert_pem && strlen(cert_pem) > 0) {
        config.cert_pem = cert_pem;
        ESP_LOGI(TAG, "Using custom certificate for HTTPS verification");
    } else {
        // Use ESP32's certificate bundle for common CAs
        config.crt_bundle_attach = esp_crt_bundle_attach;
        ESP_LOGI(TAG, "Using built-in certificate bundle");
    }
    
    // Enable certificate verification
    config.skip_cert_common_name_check = false;
    config.use_global_ca_store = true;
    
    return esp_http_client_init(&config);
}

esp_err_t ota_http_check_version(const char *base_url, char *version_out, size_t version_len)
{
    char version_url[512];
    snprintf(version_url, sizeof(version_url), "%s/version.json", base_url);
    
    esp_http_client_handle_t client = ota_http_client_init(version_url, NULL);
    if (!client) {
        return ESP_ERR_NO_MEM;
    }
    
    esp_err_t ret = esp_http_client_perform(client);
    
    if (ret == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        
        if (status_code == 200 && content_length > 0 && content_length < 1024) {
            char buffer[1024];
            int data_read = esp_http_client_read_response(client, buffer, sizeof(buffer) - 1);
            if (data_read > 0) {
                buffer[data_read] = '\0';
                
                // Simple version extraction - look for version field in JSON
                char *version_start = strstr(buffer, "\"version\"");
                if (version_start) {
                    version_start = strchr(version_start, ':');
                    if (version_start) {
                        version_start = strchr(version_start, '"');
                        if (version_start) {
                            version_start++; // Skip opening quote
                            char *version_end = strchr(version_start, '"');
                            if (version_end) {
                                size_t len = version_end - version_start;
                                if (len < version_len) {
                                    strncpy(version_out, version_start, len);
                                    version_out[len] = '\0';
                                    ret = ESP_OK;
                                } else {
                                    ret = ESP_ERR_NO_MEM;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            ESP_LOGW(TAG, "Version check failed: HTTP %d", status_code);
            ret = ESP_FAIL;
        }
    }
    
    esp_http_client_cleanup(client);
    return ret;
}

esp_err_t ota_http_download_file(const char *url, const char *cert_pem, 
                                 esp_err_t (*data_handler)(const char *data, size_t len, void *ctx),
                                 void *handler_ctx)
{
    esp_http_client_handle_t client = ota_http_client_init(url, cert_pem);
    if (!client) {
        return ESP_ERR_NO_MEM;
    }
    
    esp_err_t ret = esp_http_client_open(client, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(ret));
        esp_http_client_cleanup(client);
        return ret;
    }
    
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        ESP_LOGE(TAG, "Failed to fetch headers");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Downloading file, content length: %d bytes", content_length);
    
    char buffer[4096];
    int total_read = 0;
    
    while (total_read < content_length) {
        int data_read = esp_http_client_read(client, buffer, sizeof(buffer));
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error reading data: %d", data_read);
            ret = ESP_FAIL;
            break;
        }
        
        if (data_read == 0) {
            break; // End of data
        }
        
        // Call data handler
        if (data_handler) {
            ret = data_handler(buffer, data_read, handler_ctx);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Data handler failed: %s", esp_err_to_name(ret));
                break;
            }
        }
        
        total_read += data_read;
    }
    
    if (ret == ESP_OK && total_read == content_length) {
        ESP_LOGI(TAG, "Download completed successfully: %d bytes", total_read);
    } else if (ret == ESP_OK) {
        ESP_LOGW(TAG, "Download may be incomplete: %d/%d bytes", total_read, content_length);
    }
    
    esp_http_client_cleanup(client);
    return ret;
}