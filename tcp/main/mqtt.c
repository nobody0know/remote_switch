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
#include "ota_updata.h"

#include "can.h"
#include "pid.h"
#include "pwm.h"
#include "ir.h"
int16_t message_flag=0;
int8_t topic_num=0;
int acctr_error_code;
char switch_data[10];
char switch_topic[20];
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
    int msg_id[4];
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id[0] = esp_mqtt_client_subscribe(client, "lightleft002", 2);//lightleft002为巴法云中mqtt控制台中对应主题的名字，lightleft可换，002不可，002是巴法云小程序识别为灯的特征码
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id[0]);
        msg_id[1] = esp_mqtt_client_subscribe(client, "lightright002", 2);//同理
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id[1]);
        msg_id[2] = esp_mqtt_client_subscribe(client, "lightall002", 2);//同理
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id[2]);
        msg_id[3] = esp_mqtt_client_subscribe(client, "airconditioner001", 2);//同理
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id[3]);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id[0] = esp_mqtt_client_publish(client, "lightleft002", "start", 0, 2, 0);//左边灯开关
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id[0]);
        msg_id[1] = esp_mqtt_client_publish(client, "lightright002", "data", 0, 2, 0);//右边灯开关
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id[1]);
        msg_id[2] = esp_mqtt_client_publish(client, "lightall002", "data", 0, 2, 0);//全部的开关
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id[2]);
        msg_id[2] = esp_mqtt_client_publish(client, "airconditioner001", "data", 0, 2, 0);//全部的开关
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
        for (int i = 0; i < event->topic_len; i++)
        {
            switch_topic[i] = *event->topic++;
        }
        printf("topic = %s\n",switch_topic);
        for (int i = 0; i < event->data_len; i++)
        {
            switch_data[i] = *event->data++;
        }
        printf("DATA=%s\r\n",switch_data);
        if(my_strcmp(switch_topic,"lightleft002"))
        {
            if(my_strcmp(switch_data,"ON"))
            servo_control(11);
            else if(my_strcmp(switch_data,"OFF"))
            servo_control(12);           
        }
        else if(my_strcmp(switch_topic,"lightright002"))
        {
            if(my_strcmp(switch_data,"ON"))
            servo_control(21);
            else if(my_strcmp(switch_data,"OFF"))
            servo_control(22);
        }
        else if(my_strcmp(switch_topic,"lightall002"))
        {
            if(my_strcmp(switch_data,"ON"))
            servo_control(31);
            else if(my_strcmp(switch_data,"OFF"))
            servo_control(32);
            else if(my_strcmp(switch_data,"updata"))//对lightall002这个主题发送updata即可启动ota升级，记得将SDK配置里的版本升一级然后把bulid里的bin文件上传服务器
            ota_start_updata();
        }
        else if(my_strcmp(switch_topic,"airconditioner001"))
        {
            
            if(my_strcmp(switch_data,"ON"))
            acctr_error_code = ac_status_config(true,26,0);//开机，26摄氏度，自动风
            else if(my_strcmp(switch_data,"OFF"))
            acctr_error_code = ac_status_config(false,26,0);//关机
            printf("ac_control error code:%d\n",acctr_error_code);
        }
        memset(switch_data,'\0',sizeof(switch_data)); 
        memset(switch_topic,'\0',sizeof(switch_topic)); 
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
        .client_id = "4ea6ab40f4f64f0b80fcddf9c92453f7",//你的巴法云控制台上的私钥
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
