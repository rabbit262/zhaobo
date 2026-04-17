#ifndef _STM32_SERIAL_H_
#define _STM32_SERIAL_H_

#include <cstdint>

// Physical actions the STM32 motor controller supports.
// Values are the DATA byte in frame: 0x80 0x00 0x00 <data> 0xFF
enum Stm32Action : uint8_t {
    kActionBlink         = 0x00,
    kActionEyeRotate     = 0x01,
    kActionSmile         = 0x02,
    kActionTalk          = 0x03,
    kActionTurnHead      = 0x04,
    kActionLeftEyeBlink  = 0x05,
    kActionRightEyeBlink = 0x06,
};

class Stm32Serial {
public:
    // Call once at boot. tx_pin/rx_pin are GPIO numbers for UART1.
    // Pass rx_pin = -1 if the STM32 never sends data back.
    void Initialize(int tx_pin, int rx_pin = -1);

    // Returns true if Initialize() was called successfully.
    bool IsReady() const { return uart_num_ >= 0; }

    // Expression command (cmd=0x00): sends 0x80 0x00 0x00 <action> 0xFF
    void SendExpression(Stm32Action action);

    // Speed commands (cmd=0x01)
    void SendEyeSpeed(int8_t degrees_per_sec);   // ch=0x00
    void SendNeckSpeed(int8_t degrees_per_sec);   // ch=0x01

private:
    // Build and transmit one 5-byte frame: 0x80 <cmd> <ch> <data> 0xFF
    void SendFrame(uint8_t cmd, uint8_t ch, uint8_t data);
    int uart_num_ = -1;
};

#endif // _STM32_SERIAL_H_
