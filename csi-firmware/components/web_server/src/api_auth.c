/**
 * @file api_auth.c
 * @brief API Authentication and security implementation
 */

#include <string.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>
#include <esp_random.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <cJSON.h>
#include "app_config.h"

static const char *TAG = "api_auth";

#define API_KEY_LENGTH 32
#define TOKEN_LENGTH 64
#define MAX_TOKENS 5
#define TOKEN_EXPIRY_SECONDS 3600  // 1 hour

typedef struct {
    char token[TOKEN_LENGTH + 1];
    uint64_t expiry;
    char client_ip[16];
} auth_token_t;

static struct {
    char api_key[API_KEY_LENGTH + 1];
    auth_token_t tokens[MAX_TOKENS];
    bool api_key_required;
    bool initialized;
} auth_state = {
    .api_key_required = true,
    .initialized = false
};

/**
 * @brief Generate random API key
 */
static void generate_api_key(char *key, size_t len)
{
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (size_t i = 0; i < len - 1; i++) {
        key[i] = charset[esp_random() % (sizeof(charset) - 1)];
    }
    key[len - 1] = '\0';
}

/**
 * @brief Initialize API authentication
 */
esp_err_t api_auth_init(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("api_auth", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Try to read existing API key
    size_t key_len = sizeof(auth_state.api_key);
    err = nvs_get_str(handle, "api_key", auth_state.api_key, &key_len);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Generate new API key
        generate_api_key(auth_state.api_key, sizeof(auth_state.api_key));
        
        // Save to NVS
        err = nvs_set_str(handle, "api_key", auth_state.api_key);
        if (err == ESP_OK) {
            nvs_commit(handle);
            ESP_LOGI(TAG, "Generated new API key: %s", auth_state.api_key);
        }
    } else if (err == ESP_OK) {
        ESP_LOGI(TAG, "Loaded API key from NVS");
    }
    
    // Load auth requirement setting
    uint8_t auth_required = 1;
    nvs_get_u8(handle, "auth_required", &auth_required);
    auth_state.api_key_required = (bool)auth_required;
    
    nvs_close(handle);
    
    // Clear any existing tokens
    memset(auth_state.tokens, 0, sizeof(auth_state.tokens));
    
    auth_state.initialized = true;
    ESP_LOGI(TAG, "API authentication initialized (required: %s)", 
             auth_state.api_key_required ? "yes" : "no");
    
    return ESP_OK;
}

/**
 * @brief Get current API key
 */
const char* api_auth_get_key(void)
{
    return auth_state.api_key;
}

/**
 * @brief Set new API key
 */
esp_err_t api_auth_set_key(const char *new_key)
{
    if (!new_key || strlen(new_key) < 16) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(auth_state.api_key, new_key, sizeof(auth_state.api_key) - 1);
    auth_state.api_key[sizeof(auth_state.api_key) - 1] = '\0';
    
    // Save to NVS
    nvs_handle_t handle;
    esp_err_t err = nvs_open("api_auth", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, "api_key", auth_state.api_key);
        nvs_commit(handle);
        nvs_close(handle);
    }
    
    // Invalidate all tokens when key changes
    memset(auth_state.tokens, 0, sizeof(auth_state.tokens));
    
    ESP_LOGI(TAG, "API key updated");
    return err;
}

/**
 * @brief Enable/disable API authentication
 */
esp_err_t api_auth_set_required(bool required)
{
    auth_state.api_key_required = required;
    
    // Save to NVS
    nvs_handle_t handle;
    esp_err_t err = nvs_open("api_auth", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_u8(handle, "auth_required", required ? 1 : 0);
        nvs_commit(handle);
        nvs_close(handle);
    }
    
    ESP_LOGI(TAG, "API authentication %s", required ? "enabled" : "disabled");
    return err;
}

/**
 * @brief Generate auth token for valid API key
 */
static char* generate_auth_token(const char *client_ip)
{
    // Find empty slot or oldest token
    int slot = -1;
    uint64_t oldest_time = UINT64_MAX;
    int oldest_slot = 0;
    
    uint64_t now = esp_timer_get_time() / 1000000; // Convert to seconds
    
    for (int i = 0; i < MAX_TOKENS; i++) {
        if (auth_state.tokens[i].expiry < now) {
            slot = i;
            break;
        }
        if (auth_state.tokens[i].expiry < oldest_time) {
            oldest_time = auth_state.tokens[i].expiry;
            oldest_slot = i;
        }
    }
    
    if (slot == -1) {
        slot = oldest_slot;
    }
    
    // Generate random token
    generate_api_key(auth_state.tokens[slot].token, TOKEN_LENGTH + 1);
    auth_state.tokens[slot].expiry = now + TOKEN_EXPIRY_SECONDS;
    strncpy(auth_state.tokens[slot].client_ip, client_ip, sizeof(auth_state.tokens[slot].client_ip) - 1);
    
    return auth_state.tokens[slot].token;
}

/**
 * @brief Verify auth token
 */
static bool verify_auth_token(const char *token, const char *client_ip)
{
    uint64_t now = esp_timer_get_time() / 1000000;
    
    for (int i = 0; i < MAX_TOKENS; i++) {
        if (auth_state.tokens[i].expiry > now &&
            strcmp(auth_state.tokens[i].token, token) == 0 &&
            strcmp(auth_state.tokens[i].client_ip, client_ip) == 0) {
            // Extend token expiry on successful use
            auth_state.tokens[i].expiry = now + TOKEN_EXPIRY_SECONDS;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Extract client IP from request
 */
static void get_client_ip(httpd_req_t *req, char *ip_str, size_t len)
{
    int sockfd = httpd_req_to_sockfd(req);
    struct sockaddr_in6 addr;
    socklen_t addr_len = sizeof(addr);
    
    if (getpeername(sockfd, (struct sockaddr *)&addr, &addr_len) == 0) {
        if (addr.sin6_family == AF_INET) {
            struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
            inet_ntop(AF_INET, &addr4->sin_addr, ip_str, len);
        } else {
            inet_ntop(AF_INET6, &addr.sin6_addr, ip_str, len);
        }
    } else {
        strncpy(ip_str, "unknown", len);
    }
}

/**
 * @brief POST /api/auth - Authenticate with API key
 */
esp_err_t api_auth_handler(httpd_req_t *req)
{
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty request");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *api_key = cJSON_GetObjectItem(json, "api_key");
    if (!api_key || !cJSON_IsString(api_key)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing api_key");
        return ESP_FAIL;
    }
    
    // Verify API key
    if (strcmp(api_key->valuestring, auth_state.api_key) != 0) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Invalid API key");
        return ESP_FAIL;
    }
    
    // Get client IP
    char client_ip[46];
    get_client_ip(req, client_ip, sizeof(client_ip));
    
    // Generate auth token
    char *token = generate_auth_token(client_ip);
    
    // Send response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "token", token);
    cJSON_AddNumberToObject(response, "expires_in", TOKEN_EXPIRY_SECONDS);
    
    char *resp_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    free(resp_str);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    ESP_LOGI(TAG, "Auth token issued to %s", client_ip);
    return ESP_OK;
}

/**
 * @brief Middleware to check authentication
 */
esp_err_t api_auth_middleware(httpd_req_t *req)
{
    // Skip auth for certain endpoints
    if (strstr(req->uri, "/api/auth") != NULL ||
        strstr(req->uri, "/api/status") != NULL) {
        return ESP_OK;
    }
    
    // Skip if auth not required
    if (!auth_state.api_key_required) {
        return ESP_OK;
    }
    
    // Check for Authorization header
    char auth_header[128] = {0};
    if (httpd_req_get_hdr_value_str(req, "Authorization", auth_header, sizeof(auth_header)) != ESP_OK) {
        // Check for X-API-Key header (ESPHome style)
        if (httpd_req_get_hdr_value_str(req, "X-API-Key", auth_header, sizeof(auth_header)) != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Missing authentication");
            return ESP_FAIL;
        }
        
        // Direct API key authentication
        if (strcmp(auth_header, auth_state.api_key) != 0) {
            httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Invalid API key");
            return ESP_FAIL;
        }
        return ESP_OK;
    }
    
    // Bearer token authentication
    if (strncmp(auth_header, "Bearer ", 7) == 0) {
        char *token = auth_header + 7;
        char client_ip[46];
        get_client_ip(req, client_ip, sizeof(client_ip));
        
        if (!verify_auth_token(token, client_ip)) {
            httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Invalid or expired token");
            return ESP_FAIL;
        }
        return ESP_OK;
    }
    
    httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Invalid authorization format");
    return ESP_FAIL;
}

/**
 * @brief GET /api/auth/info - Get authentication info
 */
esp_err_t api_auth_info_handler(httpd_req_t *req)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "auth_required", auth_state.api_key_required);
    cJSON_AddStringToObject(response, "auth_type", "api_key");
    cJSON_AddArrayToObject(response, "supported_methods");
    
    cJSON *methods = cJSON_GetObjectItem(response, "supported_methods");
    cJSON_AddItemToArray(methods, cJSON_CreateString("X-API-Key"));
    cJSON_AddItemToArray(methods, cJSON_CreateString("Bearer Token"));
    
    char *resp_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    free(resp_str);
    cJSON_Delete(response);
    return ESP_OK;
}

/**
 * @brief Secure OTA update verification
 */
bool api_auth_verify_ota(const char *auth_header)
{
    if (!auth_state.api_key_required) {
        return true;
    }
    
    // Check if it's the API key directly
    if (strcmp(auth_header, auth_state.api_key) == 0) {
        return true;
    }
    
    // Check if it's a Bearer token
    if (strncmp(auth_header, "Bearer ", 7) == 0) {
        // For OTA, we don't check IP since it might come from different source
        char *token = auth_header + 7;
        uint64_t now = esp_timer_get_time() / 1000000;
        
        for (int i = 0; i < MAX_TOKENS; i++) {
            if (auth_state.tokens[i].expiry > now &&
                strcmp(auth_state.tokens[i].token, token) == 0) {
                return true;
            }
        }
    }
    
    return false;
}