/**
 * @file ir_detector.c
 * @author adarsh.raghoothaman@st.ovgu.de
 * @author elvis.joseph@st.ovgu.de
 * @brief file containing functions specific to IMU 
 * @version 0.1
 * @date 2020-07-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */


#include "sensor_node.h"

#include <inttypes.h>
#include "board.h"
#include "mpu9x50.h"
#include "mpu9x50_params.h"
#include "periph/pwm.h"
#include <assert.h>

/*PWM pin*/
#define BEEP PWM_DEV(0)
/*PWM mode*/
#define OSC_MODE PWM_LEFT
/*PWM frequency*/
#define OSC_FREQ (1000U)
/*PWM steps*/
#define OSC_STEPS (1000U)
/*change in accelerometer value greater than tap_threshold detects a tap*/
#define TAP_THRESH (200)
/*change in accelerometer value greater than lay_threshold  detects orientation change*/
#define LAY_THRESH (500)
#define SLEEP_USEC (10 * 1000u)

enum side
{
    NO_CHANGE,
    TOP,
    BOTTOM,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

mpu9x50_t dev;
mpu9x50_results_t accl, accl_prev, accl_side;
int result;

uint8_t side, side_prev = NO_CHANGE;
static char *side_names[8] = {"No_change", "Top", "Bottom", "Left", "Right", "Up", "Down"};

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
 * @brief function to get the side the board lies
 * 
 * @param accl 
 * @return uint8_t 
 */

uint8_t _get_side(mpu9x50_results_t *accl)
{
    assert(accl);
    if (accl->x_axis > LAY_THRESH)
    {
        return LEFT;
    }
    if (accl->x_axis < -LAY_THRESH)
    {
        return RIGHT;
    }
    if (accl->z_axis > LAY_THRESH)
    {
        return BOTTOM;
    }
    if (accl->z_axis < -LAY_THRESH)
    {
        return TOP;
    }

    if (accl->y_axis > LAY_THRESH)
    {
        return DOWN;
    }
    if (accl->y_axis < -LAY_THRESH)
    {
        return UP;
    }
    return NO_CHANGE;
}


/**
 * @brief function to detect tap
 * 
 * @param accl_prev 
 * @param accl 
 * @return uint8_t 
 */
uint8_t _detect_tap(mpu9x50_results_t *accl_prev, mpu9x50_results_t *accl)
{
    assert(accl_prev);
    assert(accl);

    uint8_t side = _get_side(accl_prev);
    if (side == TOP || side == BOTTOM)
    {
        if (abs(accl->z_axis) - abs(accl_prev->z_axis) > TAP_THRESH)
        {
            return 1;
        }
    }
    if (side == LEFT || side == RIGHT)
    {
        if (abs(accl->x_axis) - abs(accl_prev->x_axis) > TAP_THRESH)
        {
            return 1;
        }
    }
    if (side == UP || side == DOWN)
    {
        if (abs(accl->y_axis) - abs(accl_prev->y_axis) > TAP_THRESH)
        {
            return 1;
        }
    }
    return 0;
}


/**
 * @brief intializes MPU
 * 
 * @return int 
 */
int _mpu_init(void)
{
    accl_side.x_axis = 0;
    accl_side.y_axis = 0;
    accl_side.z_axis = 0;

    DEBUG("MPU-9X50 test application\n");

    DEBUG("+------------Initializing------------+\n");
    result = mpu9x50_init(&dev, &mpu9x50_params[0]);

    if (result == -1)
    {
        DEBUG("[Error] The given i2c is not enabled");
        return 1;
    }
    else if (result == -2)
    {
        DEBUG("[Error] The compass did not answer correctly on the given address");
        return 1;
    }

    mpu9x50_set_sample_rate(&dev, 200);
    if (dev.conf.sample_rate != 200)
    {
        DEBUG("[Error] The sample rate was not set correctly");
        return 1;
    }
    mpu9x50_set_compass_sample_rate(&dev, 100);
    if (dev.conf.compass_sample_rate != 100)
    {
        DEBUG("[Error] The compass sample rate was not set correctly");
        return 1;
    }

    DEBUG("Initialization successful\n\n");
    DEBUG("+------------Configuration------------+\n");
    DEBUG("Sample rate: %" PRIu16 " Hz\n", dev.conf.sample_rate);
    DEBUG("Compass sample rate: %" PRIu8 " Hz\n", dev.conf.compass_sample_rate);
    DEBUG("Gyro full-scale range: 2000 DPS\n");
    DEBUG("Accel full-scale range: 2 G\n");
    DEBUG("Compass X axis factory adjustment: %" PRIu8 "\n", dev.conf.compass_x_adj);
    DEBUG("Compass Y axis factory adjustment: %" PRIu8 "\n", dev.conf.compass_y_adj);
    DEBUG("Compass Z axis factory adjustment: %" PRIu8 "\n", dev.conf.compass_z_adj);

    DEBUG("Initializing beeper\n\n");
    pwm_init(BEEP, OSC_MODE, OSC_FREQ, OSC_STEPS);
    _beep_short();
    _beep_short();
    _beep_short();
    return 0;
}


void sensor_init(void)
{
    _mpu_init();
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

        mpu9x50_read_accel(&dev, &accl);

        if (_detect_tap(&accl_prev, &accl))
        {
            _beep_short();
            if (count>100)sensor_fired = true;
            puts("Tap detected");
        }

        accl_side.x_axis = (accl_side.x_axis * 0.99) + (accl.x_axis * 0.01);
        accl_side.y_axis = (accl_side.y_axis * 0.99) + (accl.y_axis * 0.01);
        accl_side.z_axis = (accl_side.z_axis * 0.99) + (accl.z_axis * 0.01);
        side = _get_side(&accl_side);

        if (side != side_prev && side != NO_CHANGE)
        {
            _beep_long();
            printf("Laying on side: %s\n", side_names[side]);
            if (count>100)
            {
                sensor_fired = true;
            }
        }
        side_prev = side;
        if(count<101) count++;
        accl_prev.x_axis = accl.x_axis;
        accl_prev.y_axis = accl.y_axis;
        accl_prev.z_axis = accl.z_axis;

        xtimer_usleep(SLEEP_USEC);
    }
}
