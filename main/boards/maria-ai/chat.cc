#include "chat.h"
#include "esp_log.h"
#include "application.h"
#include "cJSON.h"

static const char *TAG = "chat_api";

/******************************************************************
 *  WAKE XIAO (text wake)
 ******************************************************************/
static esp_err_t chat_wake_handler(httpd_req_t *req)
{
    auto &app = Application::GetInstance();

    // Mesaj hello identic cu handshake-ul WebSocket
    const char *hello =
        "{\"type\":\"hello\",\"version\":3,\"transport\":\"websocket\"}";

    app.SendChatText(hello);

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

    auto &app = Application::GetInstance();

    // Construim mesajul custom pe care Xiao îl înțelege
    cJSON *out = cJSON_CreateObject();
    cJSON_AddStringToObject(out, "type", "custom");
    cJSON_AddStringToObject(out, "payload", msg->valuestring);

    char *json_str = cJSON_PrintUnformatted(out);
    app.SendChatText(json_str);

    cJSON_free(json_str);
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
