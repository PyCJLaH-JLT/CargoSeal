# Компоненти CargoSeal

Власник: Літвінов Руслан Олексійович, ІО-36.

Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi. Фізичної плати немає. Документ описує фактичний шаблон ЛР1 і майбутню схему ЛР2 окремо. Номери сторінок нижче відповідають друкованій нумерації зазначених PDF.

## Поточна конфігурація ЛР1

| Вузол | Фактичний параметр | Підстава |
|---|---|---|
| MCU | STM32C031C6, Cortex-M0+, LQFP48, Flash 32 KiB, RAM 12 KiB | `firmware/firmware.ioc`, linker script; ST [DS13867 Rev 4](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=1), с. 1, 30 |
| Тактування | HSI 48 МГц, дільники HSI/SYSCLK/AHB/APB дорівнюють 1; Flash latency 1 | `SystemClock_Config` у `firmware/Core/Src/main.c`, значення RCC у `.ioc`; DS13867, с. 1 |
| USART2 | PA2 TX, PA3 RX, AF1, 115200 біт/с, 8N1, TX/RX, без flow control | `main.c`, `stm32c0xx_hal_msp.c`; DS13867, таблиця 13, [с. 35](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=35) |
| LED | PA5, push-pull, без pull; початково HIGH, перемикання раз на цикл | `main.h`, `MX_GPIO_Init`, цикл `main`; ST [UM2953 Rev 2](https://www.st.com/resource/en/user_manual/um2953-stm32-nucleo64-board-mb1717-stmicroelectronics.pdf#page=16), с. 16, розділ 6.10; Wokwi [Onboard LED](https://docs.wokwi.com/parts/board-st-nucleo-c031c6#onboard-led) |
| I²C1 | PB8 SCL, PB9 SDA, AF6, open-drain, GPIO_NOPULL; 7-бітна адресація; timing `0x20303E5D`, analog filter увімкнено, digital filter 0 | `main.c`, `stm32c0xx_hal_msp.c`, `.ioc`; DS13867, таблиця 15, [с. 36](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=36) |
| Кнопка | PC13, EXTI rising у конфігурації; приклад не використовує її для зміни режимів | `main.h`, `MX_GPIO_Init`; UM2953, с. 16, розділ 6.11 |
| ADC | Не ініціалізований, HAL ADC-модуль не ввімкнений; `RCC.ADCFreq_Value` у `.ioc` не означає налаштований ADC | `firmware.ioc`, `main.c`, `stm32c0xx_hal_conf.h` |
| Схема | Лише `board-st-nucleo-c031c6`; Serial Monitor RX до PA2, TX до PA3; зовнішніх датчиків немає | `diagram.json` |
| Прошивка Wokwi | HEX `firmware/build/firmware.hex`, ELF `firmware/build/firmware.elf` | `wokwi.toml`; [Wokwi project configuration](https://docs.wokwi.com/vscode/project-config), розділ Firmware and ELF |

У поточному коді `printf` передає байти через `__io_putchar` і `HAL_UART_Transmit`. Після ready кожен цикл перемикає LED, пише `tick` та виконує `HAL_Delay(1000)`. Повний період LED близько 2 с. I²C1 ініціалізований, але код не виконує читання датчика або сканування шини. Фактичну частоту SCL не вимірювали, тому timing-регістр не є доказом 100 кГц.

Відомості `.ioc` про CubeMX 6.8.1 і STM32Cube FW_C0 V1.0.1 описують походження шаблону. Поточні UART-параметри потрібно звіряти з C-кодом, оскільки `.ioc` не записує всі їх явні значення. Повторна генерація коду в ЛР2 потребує порівняння змін.

## Майбутня схема ЛР2

Наведені піни є пропозицією. Датчики та ADC ще не додані до `.ioc`, `diagram.json` або прошивки.

| Модель Wokwi | Майбутні з'єднання | Призначення та параметри моделі |
|---|---|---|
| `wokwi-mpu6050` | VCC → 3V3, GND → GND, SCL → PB8, SDA → PB9, AD0 → GND; INT не потрібен для опитування | Рух і удар контейнера. I²C адреса 0x68, AD0 HIGH змінює її на 0x69; прискорення задається у g, обертання у °/с. XDA/XCL не реалізовані. Джерело: [Wokwi MPU6050](https://docs.wokwi.com/parts/wokwi-mpu6050), Pin names, Attributes, Units. |
| `wokwi-pir-motion-sensor` | VCC → 3V3, GND → GND, OUT → PA1 як GPIO input | Активність поруч лише у режимі INSTALLED. Simulate Motion дає HIGH на 5 с, inhibitTime 1.2 с, retrigger подовжує HIGH. Це налаштування симуляційної моделі. Джерело: [Wokwi PIR](https://docs.wokwi.com/parts/wokwi-pir-motion-sensor), Attributes, Using the sensor. |
| `wokwi-photoresistor-sensor` | VCC → 3V3, GND → GND, AO → PA0 ADC_IN0, DO не підключати | Світло всередині контейнера. AO знаходиться між LDR і резистором 10 кОм; за збільшення світла AO зменшується. Початкові gamma=0.7, rl10=50 кОм при 10 lux. Джерело: [Wokwi LDR](https://docs.wokwi.com/parts/wokwi-photoresistor-sensor), Attributes, Operation. |

PA0 має ADC_IN0 за DS13867, таблиця 12, [с. 32](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=32). PA1 у цій пропозиції буде GPIO, а не аналоговим каналом. PA2/PA3 і PA5 залишаються зайнятими UART та LED. Перед генерацією перевірити pinout у CubeMX і відсутність конфліктів.

Проектний ADC має бути 12-бітним, із кодами 0..4095. Аналоговий сигнал обмежити діапазоном 0..VREF+, із номінальним живленням 3.3 В; 5 В до AO/ADC не подавати. ST описує ADC у DS13867, [с. 71, таблиця 56](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=71). 10-бітні `analogRead`-числа з Arduino-прикладу Wokwi не є конфігурацією STM32 HAL.

Для майбутньої I²C-шини передбачити зовнішні pull-up до 3V3, початково 4.7 кОм, і перевірити фронти логічним аналізатором. Це проектний вибір, а не наявна частина схеми. Частоту близько 100 кГц потрібно підтвердити після підключення; поточний timing-регістр зберігаємо без змін.

## Фізичні аналоги для дослідження

| Компонент | Параметри з документа виробника | Межа застосування до CargoSeal |
|---|---|---|
| MPU6050 | InvenSense [PS-MPU-6000A-00 Rev 3.4](https://product.tdk.com/system/files/dam/doc/product/sensor/mortion-inertial/imu/data_sheet/mpu-6000-datasheet1.pdf), с. 12–14: гіроскоп ±250/500/1000/2000 °/с, акселерометр ±2/4/8/16 g, VDD 2.375..3.46 В, типовий струм accel+gyro без DMP 3.8 мА. С. 18: I²C до 400 кГц. С. 21–22: виводи та типова схема. | Це дані мікросхеми. Напруга входу живлення й струм готового breakout залежать від його стабілізатора та обв'язки. Модель Wokwi не доводить електричну сумісність фізичного модуля. |
| PIR Panasonic EKMC1601111 | [Reference specification Rev 7.1](https://industrial.panasonic.com/cdbs/www-data/pdf/EWA0000/bltn_eng_ekmc160611_ast-ind-247327.pdf), с. 3: 3..6 В, середній струм 170 мкА, максимум 300 мкА за Iout=0, стабілізація до 30 с; с. 4: Vdd/OUT/GND; с. 6: обмеження виявлення джерел тепла. | Фізичний аналог цифрового PIR. Wokwi не заявляє, що моделює саме Panasonic; 5-секундний HIGH Wokwi не переносимо в datasheet фізичного датчика. PIR не визначає особу або стан кришки. |
| LDR Advanced Photonix PDV-P8103 | [DS PDV-P8103 Rev B](https://www.advancedphotonix.com/wp-content/uploads/2015/07/DS-PDV-P8103.pdf), с. 1–2: опір зменшується зі світлом, 16..33 кОм при 10 lux, мінімальний dark resistance 0.5 МОм, типові rise/fall 60/25 мс; два виводи пасивного резистора. | Досліджений фізичний аналог, не точна копія LDR Wokwi. Для дільника 3.3 В і 10 кОм верхня оцінка струму 0.33 мА. Реальні lux не заявляємо без калібрування. |

Мікросхема MPU6050 потребує перевірки ідентифікації, режиму живлення, діапазонів і формату відліків перед написанням драйвера. [Картка TDK](https://product.tdk.com/en/search/sensor/mortion-inertial/imu/info?part_no=MPU-6050) містить посилання Register Maps. Адреси регістрів та їх сторінки тут не наведено, бо карту регістрів у цій перевірці не вдалося прочитати. Її вивчення й перевірка WHO_AM_I залишаються кроком ЛР2. У цифрового PIR і пасивного LDR немає register map.

Номінальні струми цих аналогів малі, але сумарний бюджет живлення плати, breakout, pull-up та індикаторів фізично не перевірявся. У симуляції струм не вимірювали, зовнішнє джерело не використовується.

## Вибір компонентів і межі Wokwi

Для руху потрібні величина прискорення та обертання, тому MPU6050 корисніший за дискретну кнопку удару. Для активності поруч PIR має зміст саме після встановлення контейнера. Заміна його герконом змінила б функцію на контроль кришки. LDR з AO дає окремий ADC-вхід; цифровий датчик освітлення на I²C не забезпечив би третій інтерфейс. Потенціометр можна використати лише як тестовий сигнал ADC.

[Wokwi Nucleo-C031C6](https://docs.wokwi.com/parts/board-st-nucleo-c031c6#simulation-features) описує GPIO, USART, master I²C та ADC як підтримувані, RCC як частково реалізований, DMA як нереалізований. Тому ЛР2 планує HAL-опитування з тайм-аутами. Симуляція перевірятиме логіку й обробку сигналів, але не реальні дальність PIR, час відгуку LDR, споживання або точність IMU. У ЛР1 навіть цей запуск ще не підтверджено.

Джерело шаблону: [STM32 HAL example for Wokwi](https://github.com/wokwi/stm32-hello-wokwi). Ліцензія прикладу збережена у `LICENSE`, ліцензії ST/CMSIS у `firmware/Drivers`.
