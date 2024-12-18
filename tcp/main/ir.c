
#include "ir.h"

#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"
#include "storage.h"
#include "ir_decode.h"
#include "driver/gpio.h"

#define IR_TX_TASK_PRO 5



#define IR_TX_TASK_SIZE 2048

/* 红外载波相关宏 */
#define RMT_TX_CARRIER_EN    1   /*!< Enable carrier for IR transmitter test with IR led */

#define RMT_TX_CHANNEL    1     /*!< RMT channel for transmitter */
#define RMT_TX_GPIO_NUM  26     /*!< GPIO number for transmitter signal */
#define RMT_CLK_DIV      100    /*!< RMT counter clock divider  计数器每80M/100=1.25us计数1次*/
#define RMT_TICK_10_US    (80000000/RMT_CLK_DIV/100000)   /*!< RMT counter value for 10 us.(Source clock is APB clock) 可以定时10us*/

#define HEADER_HIGH_9000US    9000                         /*!< NEC protocol header: positive 9ms */
#define HEADER_LOW_4500US     4500                         /*!< NEC protocol header: negative 4.5ms*/
#define HEADER_HIGH_4300US    4300
#define HEADER_LOW_4300US    4300
#define HEADER_HIGH_7300US    7300
#define HEADER_LOW_5800US    5800
#define HEADER_HIGH_3000US   3000
#define HEADER_LOW_3000US   3000
#define HEADER_HIGH_4500US   4500

#define NEC_CONNECT_HIGH_US     646                         /*!< NEC protocol CONNECT: positive 0.6ms*/
#define NEC_CONNECT_LOW_US     20000                          /*!< NEC protocol CONNECT: negative 2ms*/
#define NEC_BIT_ONE_HIGH_US    646                         /*!< NEC protocol data bit 1: positive 0.56ms */
#define NEC_BIT_ONE_LOW_US    1643   /*!< NEC protocol data bit 1: negative 1.69ms */
#define NEC_BIT_ZERO_HIGH_US   646                         /*!< NEC protocol data bit 0: positive 0.56ms */
#define NEC_BIT_ZERO_LOW_US   516  /*!< NEC protocol data bit 0: negative 0.56ms */
#define NEC_BIT_END            646                         /*!< NEC protocol end: positive 0.56ms */
#define NEC_BIT_MARGIN         200                          /*!< NEC parse margin time */

#define NEC_ITEM_DURATION(d)  ((d & 0x7fff)*10/RMT_TICK_10_US)  /*!< Parse duration time from memory register value */
#define NEC_DATA_ITEM_NUM   70  /*!< NEC code item number: header + 35bit data + connect +32bit +end*/
#define RMT_TX_DATA_NUM  2    /*!< NEC tx test data number */
#define rmt_item32_tIMEOUT_US  21000   /*!< RMT receiver timeout value(us) 由于连接码的时间长度大约为20600us所以设置时间长点 */
/* 红外载波相关宏 */



/* 声明类型 */
enum ac_band
{
    band_gree = 0,
    band_meidi = 1,
    songxia = 2,
    band_dajin = 3,
    band_haier = 4,
    band_haixin = 5,
    band_aux = 6,
    band_max
};
enum ac_pro_code
{
    code_1 = 0,
    code_2 = 1,
    code_3 = 2,
    code_4 = 3,
    code_5 = 4,
   
    code_max
};

//品牌+协议编码 = 红外码库
struct AC_Control
{
    uint8_t code;
    t_remote_ac_status status;  //空调控制结构体
};




/* 声明类型 */

static const char *TAG = "ir.c";


static const int tx_channel = RMT_TX_CHANNEL;

EXT_RAM_ATTR uint16_t decoded[1024] = {0}; //红外二进制文件解码的数据

static struct AC_Control ac_handle; //空调对象（基于irext）

static uint8_t band, pro_code;

TaskHandle_t ir_tx_handle;

static SemaphoreHandle_t IR_sem;

//红外码库二进制文件的路径
const char *ir_code_lib[] = {
    //格力码库
    "/spiffs/gree/ac_gree_1.bin",
    "/spiffs/gree/ac_gree_2.bin",
    "/spiffs/gree/ac_gree_3.bin",
    "/spiffs/gree/ac_gree_4.bin",
    "/spiffs/gree/ac_gree_5.bin",

    //美的码库
    "/spiffs/meidi/ac_meidi_1.bin",
    "/spiffs/meidi/ac_meidi_2.bin",
    "/spiffs/meidi/ac_meidi_3.bin",
    "/spiffs/meidi/ac_meidi_4.bin",
    "/spiffs/meidi/ac_meidi_5.bin",

    //松下码库
    "/spiffs/songxia/ac_songxia_1.bin",
    "/spiffs/songxia/ac_songxia_2.bin",
    "/spiffs/songxia/ac_songxia_3.bin",
    "/spiffs/songxia/ac_songxia_4.bin",
    "/spiffs/songxia/ac_songxia_5.bin",

    //大金码库
    "/spiffs/dajin/ac_dajin_1.bin",
    "/spiffs/dajin/ac_dajin_2.bin",
    "/spiffs/dajin/ac_dajin_3.bin",
    "/spiffs/dajin/ac_dajin_4.bin",
    "/spiffs/dajin/ac_dajin_5.bin",

    //海尔码库
    "/spiffs/haier/ac_haier_1.bin",
    "/spiffs/haier/ac_haier_2.bin",
    "/spiffs/haier/ac_haier_3.bin",
    "/spiffs/haier/ac_haier_4.bin",
    "/spiffs/haier/ac_haier_5.bin",

    //海信码库
    "/spiffs/haixin/ac_haixin_1.bin",
    "/spiffs/haixin/ac_haixin_2.bin",
    "/spiffs/haixin/ac_haixin_3.bin",
    "/spiffs/haixin/ac_haixin_4.bin",
    "/spiffs/haixin/ac_haixin_5.bin",
    //奥克斯码库
    "/spiffs/aokesi/ac_aux_1.bin",
    "/spiffs/aokesi/ac_aux_2.bin",
    "/spiffs/aokesi/ac_aux_3.bin",
    "/spiffs/aokesi/ac_aux_4.bin",
};
/*----------------------------------------------空调功能设置----------------------------------------------------------------------*/

int ac_get_temp()
{
    return ac_handle.status.ac_temp + 16;
}
int ac_get_power()
{
    return ac_handle.status.ac_power;
}
int ac_get_wind_speed()
{
    return ac_handle.status.ac_wind_speed;
}
int ac_get_mode()
{
    return ac_handle.status.ac_mode;
}


/*
 * 设置空调编码，并保存到nvs
 */
uint8_t ac_set_code_lib(uint8_t band, uint8_t pro_code)
{
    ac_handle.code = band * 4 + pro_code;
    return nvs_save_ac_code(ac_handle.code, AC_DEFAULT);
}

/*
 * 根据ac_handle发射红外线
 */
void ac_control()
{

    xSemaphoreTake(IR_sem, portMAX_DELAY);
    xTaskNotifyGive(ir_tx_handle);
}
int ac_status_config(bool open, int temp, int speed)
{
    //set temp
    if (temp > 28 || temp < 16)
    {
        return -1;
    }
    ac_handle.status.ac_temp = temp - 16;

    //set power
    if (open)
    {
        ac_handle.status.ac_power = AC_POWER_ON;
    }
    else
    {
        ac_handle.status.ac_power = AC_POWER_OFF;
    }

    //set wind_speed
    if (speed < 0 || speed > 3)
    {
        return -1;
    }
    switch (speed)
    {
    case 0:
        ac_handle.status.ac_wind_speed = AC_WS_AUTO;
        break;
    case 1:
        ac_handle.status.ac_wind_speed = AC_WS_LOW;
        break;
    case 2:
        ac_handle.status.ac_wind_speed = AC_WS_MEDIUM;
        break;
    case 3:
        ac_handle.status.ac_wind_speed = AC_WS_HIGH;
        break;
    default:
        return -1;
    }
    ac_handle.status.ac_mode = AC_MODE_COOL;

    ac_control();
    return 0;
}
int ac_set_temp(int temp)
{
    if (temp > 28 || temp < 16)
    {
        return -1;
    }
    ac_handle.status.ac_temp = temp - 16;
    ac_control();
    return 0;
}
int ac_set_mode(int mode)
{
    ac_handle.status.ac_mode = mode;

    ac_control();
    return mode;
}
int ac_open(bool open)
{
    if (open)
    {
        ac_handle.status.ac_power = AC_POWER_ON;
    }
    else
    {
        ac_handle.status.ac_power = AC_POWER_OFF;
    }
    ac_control();
    return 0;
}

int ac_set_wind_speed(int speed)
{
    if (speed < 0 || speed > 3)
    {
        return -1;
    }
    switch (speed)
    {
    case 0:
        ac_handle.status.ac_wind_speed = AC_WS_AUTO;
        break;
    case 1:
        ac_handle.status.ac_wind_speed = AC_WS_LOW;
        break;
    case 2:
        ac_handle.status.ac_wind_speed = AC_WS_MEDIUM;
        break;
    case 3:
        ac_handle.status.ac_wind_speed = AC_WS_HIGH;
        break;
    default:
        return -1;
    }
    ac_control();
    return 0;
}

int ac_set_swing(bool open)
{
    if (open)
    {
        ac_handle.status.ac_wind_dir = AC_SWING_ON;
    }
    else
    {
        ac_handle.status.ac_wind_dir = AC_SWING_OFF;
    }
    ac_control();
    return 0;
}

/*
 * @brief 填充item的电平和电平时间 需要将时间转换成计数器的计数值 /10*RMT_TICK_10_US
 */
static void nec_fill_item_level(rmt_item32_t *item, int high_us, int low_us)
{
    item->level0 = 1;
    item->duration0 = (high_us) / 10 * RMT_TICK_10_US;
    item->level1 = 0;
    item->duration1 = (low_us) / 10 * RMT_TICK_10_US;
}
/*
 * irext_build
 * brief：通过irext构建item 使用全局变量
 */
static void irext_build(rmt_item32_t *item, size_t item_num)
{
    int i = 0;

    nec_fill_item_level(item, decoded[0], decoded[1]);
    for (i = 1; i < item_num; i++)
    {
        item++;
        nec_fill_item_level(item, decoded[2 * i], decoded[2 * i + 1]);
    }
}


/*----------------------------------------------------硬件初始化-------------------------------------------------*/
/*
 * @brief RMT transmitter initialization
 */
static void nec_tx_init()
{
    rmt_config_t rmt_tx;
    rmt_tx.channel = tx_channel;                     //rmt发射通道
    rmt_tx.gpio_num = RMT_TX_GPIO_NUM;               //rmt波形产生的引脚
    rmt_tx.mem_block_num = 2;                        //由于格力红外有70个item，所以使用2个内存块，64×2=128个item
    rmt_tx.clk_div = RMT_CLK_DIV;                    //rmt时钟分频系数
    rmt_tx.tx_config.loop_en = false;                //关闭循环发射，只发射一次
    rmt_tx.tx_config.carrier_duty_percent = 50;      //载波占空比为50
    rmt_tx.tx_config.carrier_freq_hz = 38000;        //载波频率38khz 红外
    rmt_tx.tx_config.carrier_level = 1;              //载波高电平
    rmt_tx.tx_config.carrier_en = RMT_TX_CARRIER_EN; //使能载波
    rmt_tx.tx_config.idle_level = 0;                 //空闲状态低电平
    rmt_tx.tx_config.idle_output_en = true;          //输出使能
    rmt_tx.rmt_mode = RMT_MODE_TX;                   //发射模式
    ESP_LOGI(TAG, "[ 1.1 ] config rmt");
    rmt_config(&rmt_tx); //配置rmt控制器
    ESP_LOGI(TAG, "[ 1.2 ] install rmt driver");

    //使能红外发射驱动
    rmt_driver_install(rmt_tx.channel, 0, 0);
}


/*---------------------------------------接口函数----------------------------------------------------*/


/*---------------------------------------接收发送任务函数----------------------------------------------------*/

/**
 * 红外发送任务 
 */
void rmt_ir_txTask(void *agr)
{
    rmt_item32_t *item; //发射item
    size_t size = 0;    //item所需内存
    int item_num = 0;
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //等待通知

        ESP_LOGI(TAG, "ir_tx_irext:power=%d,temperature =%d", ac_handle.status.ac_power, ac_handle.status.ac_temp);
        ESP_LOGI(TAG, "using code lib:%s", ir_code_lib[ac_handle.code]);
        //打开ac_handle的irext库二进制文件
        if (ir_file_open(REMOTE_CATEGORY_AC, SUB_CATEGORY_QUATERNARY, ir_code_lib[ac_handle.code]) != 0)
        {
            ESP_LOGI(TAG, "open file fail");
            goto tx_exit;
        }

        //根据ac_handle.status的信息解码出特定的红外序列
        uint16_t decode_len = ir_decode(KEY_AC_POWER, decoded, &ac_handle.status, 0);

        //检查序列长度
        if (decode_len > 200)
        {
            decode_len = (decode_len + 1) / 2;
        }
        //关闭irext库，释放内存
        ir_close();

        //根据红外序列构建item
        item_num = (decode_len / 2); //解码出来的序列是重复的，取一半

        size = (sizeof(rmt_item32_t) * item_num);
        item = (rmt_item32_t *)malloc(size);
        irext_build(item, item_num); //使用序列构建item

        ESP_LOGI(TAG, "write item num = %d", item_num);

        rmt_write_items(tx_channel, item, item_num, true); //将item集合写入发射通道的RAM

        rmt_wait_tx_done(tx_channel, portMAX_DELAY); //发射红外载波

        free(item); //记得释放动态分配的内存
    tx_exit:
        xSemaphoreGive(IR_sem); //释放信号量
    }
    vTaskDelete(NULL);
}
/*
 * 红外模块初始化
 * brief：初始化红外的相关变量。红外外设。创建发送和接收任务
 * 返回：1成功
*/
void IR_init()
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "init Ir");
    //初始化空调结构体的红外信息
    ac_handle.status.ac_mode = AC_MODE_COOL;
    ac_handle.status.ac_power = AC_POWER_ON;
    ac_handle.status.ac_temp = AC_TEMP_26;
    ac_handle.status.ac_wind_dir = AC_SWING_ON; //开启扫风
    ac_handle.status.ac_wind_speed = AC_WS_AUTO;

    uint8_t *temp; //缓存码库编号
    //从nvs读取码库编号
    temp = nvs_get_ac_lib(AC_DEFAULT);

    //第一次使用，nvs中无保存码库编号
    if (temp == NULL)
    {
        ESP_LOGI(TAG, "use default code lib");
        //没有编号，使用默认编号
        band = band_meidi;
        pro_code = code_4;

        if (ac_set_code_lib(band, pro_code) != ESP_OK) //保存到nvs
        {
            ESP_LOGI(TAG, "save ac code from nvs fail");
        }
        ESP_LOGI(TAG, "set ac lib to default:%u", ac_handle.code);
    }
    else
    {
        ESP_LOGI(TAG, "获取码库编码成功 code=%u", *temp);
        ac_handle.code = *temp; //成功读取nvs的数据，保存到ac_handle
        free(temp);
    }

    vSemaphoreCreateBinary(IR_sem); //创建信号量，用于同步发射接收

    nec_tx_init(); //发射器初始化

    xTaskCreate(rmt_ir_txTask, "ir_tx", IR_TX_TASK_SIZE, NULL, IR_TX_TASK_PRO, &ir_tx_handle);

    ESP_LOGI(TAG, "IR Init OK");
}
