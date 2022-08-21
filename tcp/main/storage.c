/*
 * @Author: your name
 * @Date: 2021-06-03 19:12:02
 * @LastEditTime: 2021-09-02 23:58:24
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\main\periph\storage.c
 */

#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_spiffs.h"

static const char *TAG = "STORAGE";

/*
 * 将ac_handle数据写入nvs
 * ac_handle :要保存的ac_handle
 * ac_size: ac_handle的长度
 * key 键
 */
esp_err_t nvs_save_ac_code(uint8_t code, const char *key)
{
    esp_err_t err;
    nvs_handle handle;
    size_t size = 0;

    printf("open the nvs:%s\n", AC_CODE_NAMESPACE);
    err = nvs_open(AC_CODE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        printf("nvs open fail\n");
        return err;
    }
    //item集合写入 条目
    printf("write to blob\n");
    err = nvs_set_u8(handle, key, code);
    if (err != ESP_OK)
    {
        printf("set blob fial");
        return err;
    }
    printf( "save in nvs success,key = %s,item_size = %d", key, 1);

    //提交
    
    err = nvs_commit(handle);
    if (err != ESP_OK)
    {
        printf("commit blob fial");
        return err;
    }

    //记得关闭
    nvs_close(handle);

    return ESP_OK;
}
/*
 * 从nvs中读取ac_handle
 * key 键
 * len ac_handle的字节数
 * 返回 ac_handle指针
*/
uint8_t *nvs_get_ac_lib(const char *key)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    size_t len = 0;
    nvs_handle handle;
    esp_err_t err;
    uint8_t *code;
    

    err = nvs_open(AC_CODE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "nvs open fail");
        return NULL;
    }
    code = (uint8_t *)malloc(sizeof(uint8_t));
    //检查存在
    err = nvs_get_u8(handle, key,code);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "get u8 error,err = %d,key = %s",err,key);
        free(code);
        return NULL;
    }
    printf("get code = %u from nvs", *code);
    nvs_close(handle);

    return code;
}
/*
 * 将item内的数据写入nvs
 * item ringbuff中读取的item指针
 * items_size 所有item的字节长度 一个item32位
 * name 键
 */
esp_err_t nvs_save_items(rmt_item32_t *item, size_t items_size, const char *key)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    nvs_handle items_handle;
    esp_err_t err;
    size_t size = 0;

    //以读写方式打开红外接收仓库
    err = nvs_open(IR_STORAGE_NAMESPACE, NVS_READWRITE, &items_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "nvs open fail");
        return err;
    }

    //检查条目是否已经存在
    err = nvs_get_blob(items_handle, key, NULL, &size);

    if (size != 0)
    {
        //条目已经存在 接下来覆盖之前的数据
        ESP_LOGI(TAG, "key has alreadey exist! will cover it");
    }

    //item集合写入 条目
    err = nvs_set_blob(items_handle, key, item, items_size);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "set blob fial");
        return err;
    }
    ESP_LOGI(TAG, "save in nvs success,key = %s,item_size = %d", key, items_size);

    //提交
    err = nvs_commit(items_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "commit blob fial");
        return err;
    }

    //记得关闭
    nvs_close(items_handle);

    return ESP_OK;
}
/*
 * 从nvs中读取item
 * key 键
 * item_size 返回item的字节数
 * 返回 item指针
 * 注意：item使用后要free(item);释放内存
*/
rmt_item32_t *nvs_get_items(size_t *item_size, const char *key)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    nvs_handle items_handle;
    esp_err_t err;
    rmt_item32_t *items;

    err = nvs_open(IR_STORAGE_NAMESPACE, NVS_READWRITE, &items_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "nvs open fail");
        return NULL;
    }

    //检查存在
    err = nvs_get_blob(items_handle, key, NULL, item_size);
    if (err != ESP_OK || err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "get blob error,item may be delete");
        return NULL;
    }
    if (*item_size == 0)
    {
        ESP_LOGI(TAG, "key=%s do not exist!", key);
        return NULL;
    }
    items = (rmt_item32_t *)malloc(*item_size);
    if (items == NULL)
    {
        return NULL;
    }
    //读取item
    err = nvs_get_blob(items_handle, key, items, item_size);
    if (err != ESP_OK)
    {
        return NULL;
    }

    nvs_close(items_handle);

    return items;
}
/*
 * brief 删除nvs中指定key的item
 * 返回 err状态码
 */
esp_err_t nvs_delete_items(const char *key)
{
    nvs_handle items_handle;
    esp_err_t err;
    esp_log_level_set(TAG, ESP_LOG_INFO);
    size_t size = 0;
    err = nvs_open(IR_STORAGE_NAMESPACE, NVS_READWRITE, &items_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "open nvs fail");
        return err;
    }

    //检查存在
    err = nvs_get_blob(items_handle, key, NULL, &size);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "item don't exist");
        return err;
    }
    if (size == 0)
    {
        ESP_LOGI(TAG, "nvs do not exist");
        return err;
    }
    //擦除
    nvs_erase_key(items_handle, key);

    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, " %s erase fail", key);
        return err;
    }
    ESP_LOGI(TAG, "key=%s erase success!", key);

    nvs_close(items_handle);
    return ESP_OK;
}

/*
 * 存储系统初始化
 * 包含nvs_flash，spiffs文件系统
 * return 1：成功；0：失败
 */
int storage_init()
{

    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "Initializing storage");
    esp_err_t err = nvs_flash_init(); //nvs初始化
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase()); //擦除nvs
        err = nvs_flash_init();             //重新初始化
    }

    ESP_LOGI(TAG, "Initializing SPIFFS");

    //配置spiffs文件系统
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs", //文件系统的根目录
        .partition_label = NULL,
        .max_files = 5, //最多五个文件
        .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf); //注册spiffs文件系统

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return 0;
    }

    size_t total = 0, used = 0;                 //文件系统总大小，已使用的空间
    ret = esp_spiffs_info(NULL, &total, &used); //获取spiffs系统的信息
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return 0;
    }
    else
    {
        ESP_LOGI(TAG, "Storage Init OK,Partition size: total: %d, used: %d", total, used);
    }

    return 1;
}
