# Компоненти CargoSeal
Літвінов Руслан Олексійович, ІО-36. ЛР2, форма B. Усі три сенсори підключені одночасно і працюють з однією прошивкою. Плата STM32C031C6 має 32 KiB Flash і 12 KiB RAM.

| Компонент | Реалізований інтерфейс | Призначення та межі |
|---|---|---|
| MPU6050 | I2C1 PB8/PB9 AF6, 0x68 за AD0 LOW | ±8 g, ±250 °/s; норму прискорення та гіроскопа використовує власна policy. Нахил у спокої не змінює норму 1 g. |
| PIR | PA1 input, pull-down | Один фронт NEARBY у INSTALLED, delayTime=5 s. Не доводить фізичну присутність чи відсутність нерухомої людини. |
| LDR AO | PA0 ADC1 IN0, 12-bit PCLK/4 | E=4095-raw, rolling mean 4, E0 із 20 samples. ADC-коди не є каліброваними люксами. |
| LED | PA5 LD4 | Heartbeat раз на 1 s або постійний HIGH при alarm. |
| ADC_MARK | PA4 | Межі HAL ADC call для логічного аналізатора. |
| B1 | PC13 EXTI з ЛР1 | Не перемикає MODE та не виконує ACK. |

Pinout, напруги, pull-up, power budget, таблиці datasheet та монтаж знаходяться у [pinout.md](pinout.md). PA2/PA3 є USART2/VCP, не Arduino D1/D0. Артефакти .ioc описують ADC/UART/I2C; код у USER CODE викликає Board runtime і переживає regeneration лише після перевірки diff.

Для MPU використано оригінальну InvenSense register map Rev 4.0, [PDF на SparkFun](https://cdn.sparkfun.com/datasheets/Sensors/Accelerometers/RM-MPU-6000A.pdf). WHO_AM_I 0x75=0x68; accel FS_SEL=2 у 0x1C, gyro FS_SEL=0 у 0x1B. Ініціалізація й readback підтверджені власним трафіком, 14-byte burst зіставлено з signed big-endian і scale 4096/131.

MPU6050 дає величини прискорення та обертання; дискретний ударний GPIO датчик їх не дає. PIR відповідає активності поруч, геркон відповідав би стану кришки. AO LDR дає окремий ADC-інтерфейс, DO не дає величини сигналу. Новішу IMU потрібно було б перевіряти на доступність моделі та іншу карту регістрів.

Wokwi моделює GPIO, USART, master I²C і ADC, RCC частково, DMA не реалізовано. Передавання UART використовує IRQ. У цьому запуску HAL Mem_Read зависав на TC; власний I2C1 adapter AUTOEND обійшов це, що перевірено декодером. Модель не підтверджує rise time, аналоговий шум, споживання, реальну дальність PIR або параметри breakout. Силових пристроїв, SPI-flash, PWM і камери немає.
