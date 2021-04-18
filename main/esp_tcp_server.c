/**
 * @file esp_tcp_server.c
 * @author Victor Alberti Costa (victor.alberti.costa@gmail.com)
 * @brief TCP server/client application that changes GPIO state
 * through commands using sockets
 * @version 0.1
 * @date 2021-04-17
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

static const char *TAG = "TCP Application";
static uint32_t update_status = 0;


/* Pinout used */
#define RED_LED_PIN             15
#define GREEN_LED_PIN           18
#define BLUE_LED_PIN            4


/* ID Commands recieved from client side */
#define RECV_RED_LED_ON         (1U << 0)
#define RECV_RED_LED_OFF        (1U << 1)
#define RECV_GREEN_LED_ON       (1U << 2)
#define RECV_GREEN_LED_OFF      (1U << 3)
#define RECV_BLUE_LED_ON        (1U << 4)
#define RECV_BLUE_LED_OFF       (1U << 5)


/* Process commands from client */
static void process_cmd(int sock){
    int len;
    int recv_cmd = 0;
    
    do {
        len = recv(sock, (int *)&recv_cmd, sizeof(recv_cmd), 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } 
        else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } 
        else {
            ESP_LOGI(TAG, "Received %d bytes from client with command type: %d", len, recv_cmd);

            switch (recv_cmd){

                case RECV_RED_LED_ON:
                    ESP_LOGE(TAG, "Turning ON - RED LED");
                    gpio_set_level(RED_LED_PIN, 1);
                    update_status &= ~RECV_RED_LED_ON;
                    update_status |= RECV_RED_LED_OFF;
                break;
                
                case RECV_RED_LED_OFF:
                    ESP_LOGE(TAG, "Turning OFF - RED LED");
                    gpio_set_level(RED_LED_PIN, 0);
                    update_status &= ~RECV_RED_LED_OFF;
                    update_status |= RECV_RED_LED_ON;
                break;   

                case RECV_GREEN_LED_ON:
                    ESP_LOGE(TAG, "Turning ON - GREEN LED");
                    gpio_set_level(GREEN_LED_PIN, 1);
                    update_status &= ~RECV_GREEN_LED_ON;
                    update_status |= RECV_GREEN_LED_OFF;
                break;
                
                case RECV_GREEN_LED_OFF:
                    ESP_LOGE(TAG, "Turning OFF - GREEN LED");
                    gpio_set_level(GREEN_LED_PIN, 0);
                    update_status &= ~RECV_GREEN_LED_OFF;
                    update_status |= RECV_GREEN_LED_ON;
                break;   

                case RECV_BLUE_LED_ON:
                    ESP_LOGE(TAG, "Turning ON - BLUE LED");
                    gpio_set_level(BLUE_LED_PIN, 1);
                    update_status &= ~RECV_BLUE_LED_ON;
                    update_status |= RECV_BLUE_LED_OFF;
                break;
                
                case RECV_BLUE_LED_OFF:
                    ESP_LOGE(TAG, "Turning OFF - BLUE LED");
                    gpio_set_level(BLUE_LED_PIN, 0);
                    update_status &= ~RECV_BLUE_LED_OFF;
                    update_status |= RECV_BLUE_LED_ON;
                break; 
               
            }

            /* Send a status to update the client menu */ 
            send(sock, (int *)&update_status, sizeof(update_status), 0);

        }
    } while (len > 0);
}

static void tcp_server_task(void *pvParameters){
    char addr_str[128];
    int addr_family = (int)pvParameters;
    struct sockaddr_in dest_addr;

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(3333);
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);


    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound on port 3333");

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        /* Starts in low level and initialize status */
        update_status = RECV_RED_LED_ON | RECV_GREEN_LED_ON | RECV_BLUE_LED_ON;
        gpio_set_level(RED_LED_PIN, 0);
        gpio_set_level(GREEN_LED_PIN, 0);
        gpio_set_level(BLUE_LED_PIN, 0);

        struct sockaddr_in source_addr;
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Convert ip address to string
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        process_cmd(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    /* Configure GPIO's */
    gpio_pad_select_gpio(RED_LED_PIN);
    gpio_pad_select_gpio(GREEN_LED_PIN);
    gpio_pad_select_gpio(BLUE_LED_PIN);
    gpio_set_direction(RED_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_LED_PIN, GPIO_MODE_OUTPUT);

    /* Starts in low level */
    gpio_set_level(RED_LED_PIN, 0);
    gpio_set_level(GREEN_LED_PIN, 0);
    gpio_set_level(BLUE_LED_PIN, 0);

    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}
