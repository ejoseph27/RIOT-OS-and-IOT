/**
 * @file ir_detector.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief file containing functions specific to IR sensor
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "sensor_node.h"
#include <inttypes.h>
#include "board.h"
#include "periph/pwm.h"
#include <assert.h>

/*PWM pin*/
#define BEEP PWM_DEV(0)
/*PWM mode*/
#define OSC_MODE PWM_LEFT
/*PWM frequency*/
#define OSC_FREQ (1000U)
/*PWM steps*/
#define OSC_STEPS (1000U
#define SLEEP_USEC (10 * 1000u)
#define IRPIN GPIO_PIN(PORT_A, 15)

/**
 * @brief function to produce beep for small duration
 * 
 */
void _beep_short(void)
{
    pwm_set(BEEP, 0, 50);
    xtimer_usleep(100 * 1000U);
    pwm_set(BEEP, 0, 0);
    xtimer_usleep(10 * 1000U);
}
/**
 * @brief function to produce beep for a long duration
 * 
 */
void _beep_long(void)
{
    pwm_set(BEEP, 0, 50);
    xtimer_usleep(500 * 1000U);
    pwm_set(BEEP, 0, 0);
    xtimer_usleep(500 * 1000U);
}
/**
 * @brief initialize the params for IR detector
 * 
 * @return int 
 */
int ir_init(void)
{
    DEBUG("Initializing IR sensor\n");
    gpio_init(IRPIN,GPIO_IN);
    DEBUG("Initializing beeper\n");
    pwm_init(BEEP,OSC_MODE,OSC_FREQ,OSC_STEPS));
    _beep_short();
    _beep_short();
    _beep_short();
    return 0;

}

void sensor_init(void)
{
    ir_init();
}

void sensor_detection_loop(void)
{
    DEBUG("\n+--------Starting Measurements--------+\n");
    int count = 0;

    while (1)
    {
        if (!connection_status)
        {
            puts("Sensor detection sleeping");
            count = 0;
            thread_sleep();
        }

        if (gpio_read(IRPIN) == 0)
        {
            _beep_short();
            if (count > 100)
            {
                sensor_fired = true;
                puts("Object detected");
            }
        }

        if (count < 101)
        {
            count++;
        }
        xtimer_usleep(SLEEP_USEC);
    }
}
