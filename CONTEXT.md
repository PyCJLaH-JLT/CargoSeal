# Контекст CargoSeal
Власник Літвінов Руслан Олексійович, ІО-36. Репозиторій продовжує ЛР1, форма B, фізичної плати немає.

ЛР3 додає append-only custody journal 102×40 bytes у сторінках flash 14–15, чергу 8 events, CRC і commit doubleword, TIM3 1 MHz, GPIO PA4/6/7/8, STATS/LOG STATUS/DUMP/ERASE. Постійний журнал не відновлює активний TRANSPORT/alarm після reset. ARM LTO build і host tests пройшли, власні Wokwi S1/S2/S3 нової збірки очікують перевірки. Host virtual hour не є годиною Wokwi. Деталі: docs/tasks.md, metrics/README.md, limitations.md. Старі captures є доказами тільки ЛР2.

Предметна модель: контейнер експоната, два явні режими TRANSPORT/INSTALLED, незалежний прапорець alarm. MODE змінює тільки режим і незавершені motion timers; baseline та вже зафіксовану тривогу зберігає. ACK очищує alarm після валідної норми IMU і світла 500 ms. MOVE і NEARBY інформаційні; DISTURBANCE, IMPACT та LIGHT_CHANGE тривожні. Остання подія означає можливий доступ за світловою ознакою.

Core містить HAL init/IRQ; Board зв'язує периферію з Sensors і Policy. MPU6050 driver використовує register_bus callbacks; light driver калібрує E0 і фільтрує чотири значення. Policy не включає HAL. Дати перевірок та джерела власних сигналів знаходяться у docs/verification/lab2 і docs/captures/manifest.json.

T01..T12 мають записи або відповідні host tests. Точні пороги, wrap і 20 STATUS/s належать до host tests. Wokwi підтверджує інтеграцію, 36 подій, I²C ID/burst/NACK, UART і GPIO/ADC. Фізичні характеристики моделей не доведено. Енергозалежний UART-журнал очищується після перезапуску.

Гілка feature/lab-2 продовжує main із злитою ЛР1. main та ЛР1 не змінюються. Склад компонентів, піни, команди та правила наведено у власному PRD, pinout і uart-protocol; TILT або функцій керування лампою у CargoSeal немає.
