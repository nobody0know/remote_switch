#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt.h"

#include "can.h"
#include "pid.h"
#include "pwm.h"
int16_t message_flag=0;
int8_t topic_num=0;
char switch_data[10];
static const char *TAG = "ESP32";
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}
int my_strcmp(const char* str1, const char* str2)
{
	int ret = 0;
	while(!(ret=*(unsigned char*)str1-*(unsigned char*)str2) && *str1)
	{
		str1++;
		str2++;
	}

	if (ret < 0)
	{
		return 0;
	}
	else if (ret > 0)
	{
		return 0;
	}
	return 1;
}



/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id[3];
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id[0] = esp_mqtt_client_subscribe(client, "lightleft0", 2);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id[0]);
        msg_id[1] = esp_mqtt_client_subscribe(client, "lightright1", 2);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id[1]);
        msg_id[2] = esp_mqtt_client_subscribe(client, "lightall2", 2);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id[2]);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id[0] = esp_mqtt_client_publish(client, "lightleft0", "data", 0, 2, 0);//左边灯开关
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id[0]);
        msg_id[1] = esp_mqtt_client_publish(client, "lightright1", "data", 0, 2, 0);//右边灯开关
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id[1]);
        msg_id[2] = esp_mqtt_client_publish(client, "lightall2", "data", 0, 2, 0);//全部的开关
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id[2]);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        message_flag = 0;
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        //printf("DATA=%.*s\r\n", event->data_len, event->data);
        char switch_topic = *(event->topic+(event->topic_len-1));
        printf("topic = %c\n",switch_topic);
        
        for (int i = 0; i < event->data_len; i++)
        {
            switch_data[i] = *event->data++;
        }
        printf("DATA=%s\r\n",switch_data);
            
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

 void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = "bemfa.com",
        .port = 9501,
        .client_id = "4ea6ab40f4f64f0b80fcddf9c92453f7",
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
