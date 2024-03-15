#ifndef LED_ROUTINE_H
#define LED_ROUTINE_H

/**
 * @fn int init_led()
 *
 * @brief initialize GPIO Pin for led
 * 
 * @return NRC_SUCCES on success
 **/
int init_led();

/**
 * @fn int set_pin_on_led()
 *
 * @brief light led 
 * 
 * @return NRC_SUCCES on success 
 **/
int set_pin_on_led();

/**
 * @fn int reset_pin_on_led()
 *
 * @brief unlight led 
 * 
 * @return NRC_SUCCES on success 
 **/
int reset_pin_on_led();

#endif