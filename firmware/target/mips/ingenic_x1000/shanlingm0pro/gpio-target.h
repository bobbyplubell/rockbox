/*              Name            Port    Pins            Function */
DEFINE_PINGROUP(LCD_DATA,       GPIO_A, 0xffff <<  0,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(LCD_CONTROL,    GPIO_B,   0x1a << 16,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(MSC0,           GPIO_A,   0x3f << 20,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(SFC,            GPIO_A,   0x3f << 26,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(I2S,            GPIO_B,   0x1f <<  0,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(I2C0,           GPIO_B,      3 << 23,   GPIOF_DEVICE(0))
DEFINE_PINGROUP(I2C1,           GPIO_C,      3 << 26,   GPIOF_DEVICE(0))
DEFINE_PINGROUP(I2C2,           GPIO_D,      3 <<  0,   GPIOF_DEVICE(1))

/* Device GPIOs — confirmed from stock firmware GPIO register dumps. */

/* touch IRQ, USB detect, touch reset */
DEFINE_GPIO(HYNITRON_INTERRUPT, GPIO_PA(16),    GPIOF_INPUT)
DEFINE_GPIO(USB_DETECT,         GPIO_PA(17),    GPIOF_INPUT)
DEFINE_GPIO(HYNITRON_RESET,     GPIO_PA(19),    GPIOF_OUTPUT(0))

/* LCD power, SD card detect */
DEFINE_GPIO(LCD_PWR,            GPIO_PB(6),     GPIOF_OUTPUT(1))
DEFINE_GPIO(MSC0_CD,            GPIO_PB(9),     GPIOF_INPUT)

/* DAC1_HP_EN (PB10) and DAC2_HP_EN (PB11) not configured here;
 * managed by audiohw to avoid gpio_init pop. */

/* DAC power, LCD control, USB VBUS, power button */
DEFINE_GPIO(ES9218_POWER,       GPIO_PB(13),    GPIOF_OUTPUT(0))
DEFINE_GPIO(LCD_RST,            GPIO_PB(15),    GPIOF_OUTPUT(1))
DEFINE_GPIO(LCD_RD,             GPIO_PB(16),    GPIOF_OUTPUT(1))
DEFINE_GPIO(LCD_CE,             GPIO_PB(18),    GPIOF_OUTPUT(1))
DEFINE_GPIO(USB_DRVVBUS,        GPIO_PB(25),    GPIOF_OUTPUT(0))
DEFINE_GPIO(BTN_POWER,          GPIO_PB(31),    GPIOF_INPUT)

/* PMIC IRQ */
DEFINE_GPIO(AXP_IRQ,            GPIO_PC(21),    GPIOF_INPUT)

/* rotary encoder, DAC GPIO2, DAC reset */
DEFINE_GPIO(WHEEL1,             GPIO_PD(2),     GPIOF_INPUT)
DEFINE_GPIO(WHEEL2,             GPIO_PD(3),     GPIOF_INPUT)
DEFINE_GPIO(ES9218_GPIO2,       GPIO_PD(4),     GPIOF_INPUT)
DEFINE_GPIO(ES9218_RESET,       GPIO_PD(5),     GPIOF_OUTPUT(0))
