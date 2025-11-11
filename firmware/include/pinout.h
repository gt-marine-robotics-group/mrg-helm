#include "board_config.h"

/*
This is where you should define all physical connection to the board
In theory if you ever did a board redesign and the specific pins changes
with the same sensors, this should be the only file in the code you have
to touch.

We use the namespace pins so in the firmware we can just say:

pins::STATUS_LED

Feel free to take this namespace out if you prefer its just a practice I
personally follow
*/


namespace pins {
    // ------- General Pi Pico Pins ------
    constexpr uint8_t PICO_STATUS_LED = 25; // TODO: Confirm this is correct
    // TODO: Any other general Pi Pico pins that are the same across boards

    // ------- Presto Board Pinout ------
    #if defined(BOARD_PRESTO) && BOARD_PRESTO
        

        #if defined(HAS_MOTORS) && HAS_MOTORS
            // TODO: Fill in Presto Motor pins

            /*
            Example:
            constexpr uint8_t MOTOR_L1_PWM = 23;
            constexpr uint8_t SERVO_2 = 22;
            constexpr uint8_t SERVO_3 = 21;
            constexpr uint8_t SERVO_4 = 20;
            constexpr uint8_t SERVO_5 = 19;
            constexpr uint8_t SERVO_6 = 18;

            I recommend using constexpr for these instead of just
            #define so it can do type checking

            be intentional about the name. Don't just put 
            Motor_1 Motor_2 etc. make the name more descriptive
            so Motor_1_PWM would be better and if you can name
            the motors to be less ambiguous about where they are
            even better
            */

        #endif

        #if defined(HAS_INDICATOR_LED) && HAS_INDICATOR_LED
            // TODO: Fill in Presto E_Stop pins

            /*
            I recommend using constexpr for these instead of just
            #define so it can do type checking
            */

        #endif


        #if defined(HAS_E_STOP) && HAS_E_STOP
            // TODO: Fill in Presto E_Stop pins

            /*
            I recommend using constexpr for these instead of just
            #define so it can do type checking
            */

        #endif


        #if defined(HAS_AUTONOMY_SWITCH) && HAS_AUTONOMY_SWITCH
            // TODO: Fill in Presto Autonomy_Switch pins

            /*
            I recommend using constexpr for these instead of just
            #define so it can do type checking
            */

        #endif
    #elif defined(BOARD_SENSOR) && BOARD_SENSOR

        #if defined(HAS_POWER_SENSING) && HAS_POWER_SENSING
            // TODO: Fill in power sensing pins

            /*
            I recommend using constexpr for these instead of just
            #define so it can do type checking
            */

        #endif

        #if defined(HAS_PRESSURE_SENSING) && HAS_PRESSURE_SENSING
            // TODO: Fill in pressure sensing pins

            /*
            I recommend using constexpr for these instead of just
            #define so it can do type checking
            */

        #endif
    #else
        #error "No board macro defined: set BOARD_PRESTO or BOARD_SENSOR in platformio.ini"
    #endif
}