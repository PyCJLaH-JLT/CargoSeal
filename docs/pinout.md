# Підключення CargoSeal

Літвінов Руслан Олексійович, ІО-36. Форма B: Ubuntu 24.04, Docker, STM32 HAL, Wokwi. Плата та історія ЛР1 збережені. Усі сенсори працюватимуть в одній прошивці.

| Пін МК | AF / периферія | Arduino | Пристрій | Рівень / підтяжка | Підстава вибору |
|---|---|---|---|---|---|
| PA0 | analog / ADC1 IN0 | CN8.1 / A0 | LDR AO | 0..3.3 В, без pull | DS13867 табл. 12; окремий аналоговий канал |
| PA1 | GPIO input | CN8.2 / A1 | PIR OUT | 3.3 В, MCU pull-down | DS13867 табл. 12; не займає USART2 |
| PB8 | AF6 / I2C1 SCL | CN5.10 / D15 | MPU6050 SCL | OD, зовнішня 4.7 кОм до 3V3 | DS13867 табл. 15; шина ЛР1 |
| PB9 | AF6 / I2C1 SDA | CN5.9 / D14 | MPU6050 SDA | OD, зовнішня 4.7 кОм до 3V3 | DS13867 табл. 15; пара до PB8 |
| PA2 | AF1 / USART2 TX | VCP USART2, окремий PA2 | Monitor RX | 3.3 В, без pull | DS13867 табл. 13; журнал ЛР1 |
| PA3 | AF1 / USART2 RX | VCP USART2, окремий PA3 | Monitor TX | 3.3 В, без pull | DS13867 табл. 13; LF-команди |
| PA5 | GPIO output | D13 / LD4 | LED плати | push-pull | UM2953 §6.10; тривога та heartbeat |
| PA4 | GPIO output | CN8.3 / A2 | ADC_MARK, аналізатор D4 | push-pull | Вільний вивід для вимірювання виклику ADC |
| PC13 | GPIO input / EXTI13 | B1 | Кнопка плати | конфігурація ЛР1 | Не керує режимами; натискання не змінює стан |
| PA13/PA14 | SWD | debug | Налагодження | конфігурація ЛР1 | Не використовуються сенсорами |

Порядок монтажу: спільна земля; MPU і LDR VCC до 3V3.1; PIR VCC до 5V.1, OUT моделі 3.3 В; MPU AD0 до GND.1. AO до PA0, DO вільний. Додати по одному 4.7 кОм від SDA/SCL до 3V3.1. Monitor TX до PA3, RX до PA2. Аналізатор D0=SCL, D1=SDA, D2=UART_TX, D3=PIR, D4=ADC_MARK, D5=UART_RX, D6=LED; GND спільна.

На фізичному PIR перевірити OUT до підключення. Не подавати 5 В на ADC. Врахувати резистори breakout, перш ніж додавати зовнішні pull-up. Числа струмів нижче є оцінками, симулятор їх не вимірює.

UM2953, таблиця 11 і рисунок 7, с. 19–20, підтверджують CN8 та CN5. D0/D1 за замовчуванням є PB7/PB6 USART1, а не PA3/PA2 USART2. Для CargoSeal використовуються саме PA2/PA3 та VCP USART2. У таблиці 12 UM підписи PB8/PB9 SCL/SDA переставлено; функції звірено з DS13867 та таблицею 11 UM.

Карту регістрів InvenSense RM-MPU-6000A-00 Rev 4.0 прочитано у [копії оригінального документа на SparkFun](https://cdn.sparkfun.com/datasheets/Sensors/Accelerometers/RM-MPU-6000A.pdf). Діапазони описано на с. 14–15, дані та масштаби на с. 30–32, WHO_AM_I на с. 46. Сайт TDK для посилання Register Map повернув HTML замість PDF, тому версію 4.2 не заявляємо як прочитану.

| Вузол | Робочий струм / бюджет | Обмеження |
|---|---|---|
| MPU6050 | типово 3.8 мА кристала; резерв breakout 10 мА | PS-MPU-6000A-00 §6; maximum модуля залежить від обв'язки |
| LDR дільник 10 кОм | до 3.3/10000 = 0.33 мА; резерв модуля 5 мА | Не калібрований люксметр |
| Дві I²C підтяжки | до 2·3.3/4700 = 1.40 мА | Обидві лінії LOW |
| PIR | резерв 5V 10 мА для модуля | Фізична модель не вибрана; Wokwi не доводить maximum |

Додатковий проєктний бюджет 3V3 = 16.4 мА. UM2953 §6.5.2 задає 500 мА сумарного виходу регулятора з платою. Струм плати не виміряно, гарантований запас невідомий. Силових виконавчих пристроїв немає.

I²C адреса MPU 0x68 за AD0 LOW, 0x69 за HIGH. WHO_AM_I (0x75) очікується 0x68 незалежно від AD0. Інших адресних пристроїв немає. PIR/LDR не мають регістру ID.

HSI, SYSCLK, PCLK = 48 МГц. TIMINGR 0x20303E5D: PRESC=2, SCLDEL=3, SDADEL=0, SCLH=62, SCLL=93. Програмована тривалість high+low = (63+94)·62.5 нс = 9.8125 мкс; повний період включає фільтри та синхронізацію, його необхідно виміряти. UART 115200 8N1: BRR близько 417, біт близько 8.68 мкс. ADC PCLK/4=12 МГц, 12 біт, sample 160.5 циклів; (160.5+12.5)/12 МГц = 14.42 мкс без overhead. ADC_MARK охоплює весь HAL-виклик, а не лише conversion.

Відхилено LDR на PA5 через бортовий LED, PIR на PA3 через UART RX, перенесення I²C на PA9/PA10 через непотрібну зміну перевіреної шини. DMA, SPI, PWM, камера не входять до PRD CargoSeal. У формі B камера не моделюється; SPI-flash та її write-enable тести незастосовні до цього складу.

Джерела: [DS13867](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf), [UM2953](https://www.st.com/resource/en/user_manual/um2953-stm32-nucleo64-board-mb1717-stmicroelectronics.pdf), [RM0490](https://www.st.com/resource/en/reference_manual/rm0490-stm32c0x1-advanced-armbased-32bit-mcus-stmicroelectronics.pdf), [MPU register map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf), [Wokwi плата](https://docs.wokwi.com/parts/board-st-nucleo-c031c6), [MPU](https://docs.wokwi.com/parts/wokwi-mpu6050), [PIR](https://docs.wokwi.com/parts/wokwi-pir-motion-sensor), [LDR](https://docs.wokwi.com/parts/wokwi-photoresistor-sensor).
