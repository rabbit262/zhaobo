#include "stm32_serial.h"

#include <driver/uart.h>
#include <esp_log.h>

#define TAG "Stm32Serial"

void Stm32Serial::Initialize(int tx_pin, int rx_pin) {
    uart_num_ = UART_NUM_1;

    uart_config_t cfg = {};
    cfg.baud_rate  = 115200;
    cfg.data_bits  = UART_DATA_8_BITS;
    cfg.parity     = UART_PARITY_DISABLE;
    cfg.stop_bits  = UART_STOP_BITS_1;
    cfg.flow_ctrl  = UART_HW_FLOWCONTROL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    esp_err_t err = uart_driver_install(uart_num_, /*rx_buf=*/256, /*tx_buf=*/0,
                                        /*queue_size=*/0, /*queue=*/NULL, /*flags=*/0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        uart_num_ = -1;
        return;
    }

    uart_param_config(uart_num_, &cfg);
    uart_set_pin(uart_num_, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "Initialized UART%d  TX=GPIO%d  RX=GPIO%d  @ 115200 baud",
             uart_num_, tx_pin, rx_pin);
}

void Stm32Serial::SendFrame(uint8_t cmd, uint8_t ch, uint8_t data) {
    if (uart_num_ < 0) return;

    const uint8_t frame[5] = { 0x80, cmd, ch, data, 0xFF };
    uart_write_bytes(uart_num_, frame, sizeof(frame));

    ESP_LOGI(TAG, "TX [%02X %02X %02X %02X %02X]",
             frame[0], frame[1], frame[2], frame[3], frame[4]);
}

void Stm32Serial::SendExpression(Stm32Action action) {
    SendFrame(/*cmd=*/0x00, /*ch=*/0x00, static_cast<uint8_t>(action));
}

void Stm32Serial::SendEyeSpeed(int8_t speed) {
    SendFrame(/*cmd=*/0x01, /*ch=*/0x00, static_cast<uint8_t>(speed));
}

void Stm32Serial::SendNeckSpeed(int8_t speed) {
    SendFrame(/*cmd=*/0x01, /*ch=*/0x01, static_cast<uint8_t>(speed));
}
