/*
 * Axsem AX8052 Example Project
 */

#include <ax8052.h>
#include <libmftypes.h>
#include <libmfflash.h>
#include <libmfwtimer.h>
[LCD_HEADER][BOARD_HEADER]

struct wtimer_desc __xdata wtdesc;
uint8_t ledsave;

void wtcallback(struct wtimer_desc __xdata *desc)
{
    wtdesc.time += 320;
    wtimer0_addabsolute(&wtdesc);
    led0_toggle();
}


#if defined(__ICC8051__)
#define coldstart 1
#define warmstart 0
//
// If the code model is banked, low_level_init must be declared
// __near_func elsa a ?BRET is performed
//
#if (__CODE_MODEL__ == 2)
__near_func __root char
#else
__root char
#endif
__low_level_init(void) @ "CSTART"
#else
#define coldstart 0
#define warmstart 1
uint8_t _sdcc_external_startup(void)
#endif
{
    DPS = 0;
    wtimer0_setclksrc(CLKSRC_LPOSC, 1);
    wtimer1_setclksrc(CLKSRC_FRCOSC, 7);
[BOARD_CONFIG]
    EIE = 0x00;
    E2IE = 0x00;
    IE = 0x00;
[LCD_PORTINIT]    GPIOENABLE = 1;

    if (PCON & 0x40)
        return warmstart;
    return coldstart;
}

#undef coldstart
#undef warmstart

#if defined(SDCC)
extern uint8_t _start__stack[];
#endif

void main(void)
{
[LCD_BANNER]
#if !defined(SDCC) && !defined(__ICC8051__)
    _sdcc_external_startup();
#endif

#if defined(SDCC)
    __asm
    G$_start__stack$0$0 = __start__stack
    .globl G$_start__stack$0$0
    __endasm;
#endif

    flash_apply_calibration();
    CLKCON = 0x00;
    wtimer_init();
    if (!(PCON & 0x40)) {
[LCD_INIT]        wtdesc.time = 320;
        wtdesc.handler = wtcallback;
        wtimer0_addrelative(&wtdesc);
    }

    EA = 1;

    if (!(PCON & 0x40)) {
        // cold start
[LCD_WRBANNER]
    }

    // Insert code

    PCON = 0x0C;
    for (;;) {
        wtimer_runcallbacks();
        EA = 0;
        {
            uint8_t flg = WTFLAG_CANSTANDBY;
[LCD_IDLEFLAGS]            wtimer_idle(flg);
        }
        EA = 1;
    }
}
