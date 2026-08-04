#include "chat.h"
#include "esp_log.h"
#include "cJSON.h"
#include "mcp_server.h"

static const char *TAG = "chat_api";

/******************************************************************
 *  WAKE XIAO (text wake)
 ******************************************************************/
static esp_err_t chat_wake_handler(httpd_req_t *req)
{
    // Trimitem exact mesajul hello folosit de audio
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "hello");
    cJSON_AddNumberToObject(root, "version", 3);
    cJSON_AddStringToObject(root, "transport", "udp");

    McpServer::GetInstance().SendJson(root);
    cJSON_Delete(root);

    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

/******************************************************************
 *  SEND TEXT MESSAGE
 ******************************************************************/
static esp_err_t chat_send_handler(httpd_req_t *req)
{
    char buf[512];
    int len = httpd_req_recv(req, buf, sizeof(buf));
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid body");
        return ESP_FAIL;
    }

    buf[len] = 0;

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *msg = cJSON_GetObjectItem(root, "message");
    if (!cJSON_IsString(msg)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing message");
        return ESP_FAIL;
    }

    // Trimitem textul către Xiao prin MCP
    cJSON *out = cJSON_CreateObject();
    cJSON_AddStringToObject(out, "type", "custom");
    cJSON_AddStringToObject(out, "payload", msg->valuestring);

    McpServer::GetInstance().SendJson(out);

    cJSON_Delete(out);
    cJSON_Delete(root);

    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

/******************************************************************
 *  REGISTER ROUTES
 ******************************************************************/
void chat_register_routes(httpd_handle_t server)
{
    static const httpd_uri_t wake_uri = {
        .uri = "/chat/wake",
        .method = HTTP_POST,
        .handler = chat_wake_handler,
        .user_ctx = NULL
    };

    static const httpd_uri_t send_uri = {
        .uri = "/chat/send",
        .method = HTTP_POST,
        .handler = chat_send_handler,
        .user_ctx = NULL
    };

    ESP_LOGI(TAG, "Registering chat API routes");
    httpd_register_uri_handler(server, &wake_uri);
    httpd_register_uri_handler(server, &send_uri);
}
