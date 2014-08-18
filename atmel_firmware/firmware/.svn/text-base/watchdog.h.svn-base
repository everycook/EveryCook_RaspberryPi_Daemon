#include "mytypes.h"

/*
MCUSR – MCU Status Register
• Bit 4 – JTRF: JTAG Reset Flag
• Bit 3 – WDRF: Watchdog System Reset Flag
• Bit 2 – BORF: Brown-out Reset Flag
• Bit 1 – EXTRF: External Reset Flag
• Bit 0 – PORF: Power-on Reset Flag
*/
extern uint8_t mcusr_mirror;

extern boolean isStartup;
extern boolean wdtRestart;
extern boolean wdtRestartLast;
extern uint8_t wdtRestartCount;
extern boolean isStartup;
extern boolean isExternalReset;
extern boolean isMaintenanceMode;

void initWatchDog();

#define NODATA_TIMEOUT_SAFTY_CHAIN_OFF 10000
#define NODATA_TIMEOUT_RESET_BUTTON  20000
#define NODATA_TIMEOUT_POWER_RESET 30000

#define DAEMON_RESET_BUTTON_TIME 40000
#define DAEMON_RESET_BUTTON_IGNORE_TIME 70000
#define RASPI_RESET_BUTTON_TIME 100
#define RASPI_RESET_TIME 1000

#define DAEMON_RESET_WAIT_TIME 20000
#define RASPI_RESET_WAIT_TIME 60000

extern uint16_t lastDataTime;
extern uint16_t resetAtmelTime;
extern uint16_t resetDaemonTime;
extern uint16_t resetRaspiTime;

void triggerWatchDog(boolean pingFromDaemon);

//ISR(WDT_vect);