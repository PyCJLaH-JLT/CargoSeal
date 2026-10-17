# Залежності та збірка

ST HAL C0 1.0.1 і CMSIS успадковано з ЛР1, commit df3e3cc. ADC/ADCEx headers/sources та LL_ADC додано з офіційного ST tag v1.0.1, commit 24999b605d0b9da3e4675c4c884036ae724bb59f. Ліцензії ST збережено. Вендорський код не редагується.

ADC потрібний для PA0, I2C для MPU6050, UART для журналу/команд, GPIO для PIR/LED/ADC_MARK. RCC/FLASH/PWR/CORTEX потрібні clock init і SysTick. EXTI збережено для B1 основи ЛР1. DMA лишається залежністю HAL types, передавання DMA немає. SPI/TIM/RTC вимкнено.

Власний C-код має -Wall -Wextra -Werror, vendor code -Wall. Переносимі Sensors/Policy не включають HAL; tests використовують callbacks тієї самої шини. Власних сторонніх runtime бібліотек немає, libm забезпечує норму векторів.

Board/runtime.c виконує опитування IMU/PIR кожні 20 мс, ADC кожні 50 мс. Тайм-аут виклику I2C 10 мс, ADC 2 мс. IMU повторно ініціалізується раз на 1 с. Передавання UART здійснює HAL interrupt mode, черга TX 4096 байтів, RX 512 байтів. STATUS показує втрати. Секундного HAL_Delay немає, tick і LED heartbeat збережено за відсутності alarm; alarm тримає LED постійно HIGH.

Прошивка має обмеження Flash 32 KiB/RAM 12 KiB. Розмір bss містить зарезервований heap/stack і не є виміром найгіршого runtime stack. Відтворюваність package environment перевіряється окремо від функціональності.
