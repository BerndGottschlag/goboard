/**
 * @defgroup config Keyboard configuration.
 * @addtogroup config
 * Various settings to configure the keyboard's behavior.
 * @{
 */
#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include "mode_switch.h"

/**
 * If 1, the code assumes a simple prototype setup with just one key.
 */
#define CONFIG_TEST_SETUP 0
/**
 * If 1, the code assumes that a numpad can be connected.
 */
#define CONFIG_NUMPAD 1

/**
 * Configuration which is selected by the first switch position.
 */
#define CONFIG_FIRST_PROFILE MODE_SWITCH_UNIFYING1
/**
 * Configuration which is selected by the second switch position.
 */
#define CONFIG_SECOND_PROFILE MODE_SWITCH_BT1

#endif
/**
 * @}
 */

