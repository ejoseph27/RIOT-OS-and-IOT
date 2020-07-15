/**
 * @file beeper.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief  file contaings functions for beeper 
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "actuator_node.h"

void actuator_init(void)
{

    pwm_init(BEEP, OSC_MODE, OSC_FREQ, OSC_STEPS);
    return;
}

void actuator_fire(void)
{

    _beep_long();
    return;
}

void _beep_long(void)
{
    pwm_set(BEEP, 0, 50);
    xtimer_usleep(500 * 1000U);
    pwm_set(BEEP, 0, 0);
    xtimer_usleep(500 * 1000U);
    return;
}