# Журнал проблем CargoSeal

Фактичні перевірки виконано 13 жовтня 2026. Посилання ведуть до етапу, який містить виправлення. Числові policy tests не виявили додаткових дефектів класифікації; наведені помилки знайдено збіркою та інтеграційною перевіркою.

| Симптом | Причина | Виправлення |
|---|---|---|
| HAL ADC headers відсутні | Шаблон ЛР1 не містив ADC модуля. Увімкнення define без файлів не компілювалось. | Додано офіційні HAL C0 v1.0.1 ADC/ADCEx/LL headers та sources, налаштовано PA0. [Коміт](https://github.com/PyCJLaH-JLT/CargoSeal/commit/0fc3a284e30064069360e06117a109ddba323f9b) |
| Малий запас Flash у першій integration build | Дубльований printf ready залишав зайвий шлях виводу, text=30432 байти. | Ready передається через одну interrupt UART queue; text фінального firmware=25944 байти. [Коміт](https://github.com/PyCJLaH-JLT/CargoSeal/commit/0fc3a284e30064069360e06117a109ddba323f9b) |
| MPU init давав timeout у Wokwi | HAL Mem_Read отримував ACK D0/75, потім очікував TC, якого модель не дала. | Board I2C1 AUTOEND write STOP/read START; TX/STOP guard; ID=68 і всі 14 байтів підтверджено на шині. [Коміт](https://github.com/PyCJLaH-JLT/CargoSeal/commit/6eb1f5d0376199450d86cd1720b9d6f0f6d0c63b) |
| Decoder показував EB замість ID68 | Сортування одночасних переходів VCD за значенням змінювало останній рівень. | Зберігається фінальний рівень timestamp, додано regression test і звірку незалежним sigrok. [Коміт](https://github.com/PyCJLaH-JLT/CargoSeal/commit/6eb1f5d0376199450d86cd1720b9d6f0f6d0c63b) |
| Coverage build падав із -Werror | Після перейменування test main компілятор вимагав явне return0 у звичайній функції. | У всіх test entry points додано return0; один instrumented module object на всі suites. [Коміт](https://github.com/PyCJLaH-JLT/CargoSeal/commit/de0ad5884b5e22ffd248e5d26c46da1fe4fe8996) |
| Host cc відсутній після повторного Docker запуску | Спільний тег build image був перезаписаний іншим проектом. | Окремий cargoseal-build:lab2, GCC/libc6-dev/python3; Linux suites проходять. [Коміт](https://github.com/PyCJLaH-JLT/CargoSeal/commit/de0ad5884b5e22ffd248e5d26c46da1fe4fe8996) |
| CI попереджав про Node20 | actions/checkout v4 використовував старий Node runtime. | Pinned checkout v6.1.0 з Node24; фактичний SHA залежності збережено у workflow. [Коміт](https://github.com/PyCJLaH-JLT/CargoSeal/commit/6eb1f5d0376199450d86cd1720b9d6f0f6d0c63b) |
| Помилкове позначення UART на Arduino D0/D1 | На MB1717 D0/D1 є PB7/PB6 USART1, тоді як CargoSeal використовує USART2 PA3/PA2. | Pinout вказує VCP USART2 і PA2/PA3; звірено UM2953 table11 та DS13867. [Коміт](https://github.com/PyCJLaH-JLT/CargoSeal/commit/6eb1f5d0376199450d86cd1720b9d6f0f6d0c63b) |
