# Реєстр обмежень CargoSeal

Стан 30.10.2026: збірка 90392f53-2e36b428364c і власні S1/S2/S3 у Wokwi. Host-модель, симуляцію й фізичний контракт MCU розрізняємо. Фізичної плати немає; частина пунктів є архітектурними межами або гіпотезами, а не виміряними дефектами.

## 1. Flash для коду конкурує з історією контейнера

Симптом: перша збірка з 4 KiB журналом перевищила FLASH region на 1208 bytes. Доказ: локальний `Lab03/Work/build-first.log`; поточні `metrics/lab3-build/build.log`, sections, map. Причина: 32 KiB фізично спільні для коду і постійних записів, а не помилка класифікації. Обхід без ОС: -Os і LTO зменшили код, зберігши MCU й 102 records; складніше зіставляти .su з production. ОС додасть код/TCB/стеки; це обмеження вона посилить. Гіпотеза ЛР4: те саме призначення 4 KiB під журнал і ті самі FR помістяться лише після виміру приросту; ціль приросту flash <=1284 bytes щодо поточного ELF, інакше 28 KiB буде перевищено. Поточний code flash 27388 з 28672 bytes; manifest і build.log фіксують збірку.

## 2. UART backpressure обмежує доступ до свідчень

Симптом: при 80 STATUS/s модель втрачає 130 відповідей за 30 s, TX peak 2047 bytes; при 60/s втрат немає. Доказ: `metrics/host-model/RATE-60.log`, RATE-80.log та manifest, фактичний C runtime з fake HAL. Причина: 115200 8N1 дає максимум 11520 bytes/s, черга кінцева, обробка команд випереджає TX. Це межа моделі, а не виміряна межа плати. Без ОС: rate limit, коротший STATUS, пріоритет подій, credit protocol; додатковий стан і ризик starvation. ОС: blocking queue і окрема TX task керують backpressure, але не збільшать baud rate. Ціна: queue RAM, task stack і політика пріоритетів. Гіпотеза ЛР4: при тому самому bitrate точка стійкої втрати лишиться близькою, а втрати EVENT за пріоритетної черги мають зменшитись; перевірити 30 s при 80/s та однаковій довжині відповідей.

## 3. Foreground не витісняє довгу операцію

Симптом: у S3 host-моделі 60 блоків по 40 virtual ms дають IMU jitter max +40000 us і 119 пропущених слотів; команда, прийнята посередині erase, чекає до 20000 us до dispatch. Доказ: `metrics/host-model/S3.log`, `tests/test_runtime.c:during_erase`. Час 40 ms задано тестом, не виміряно. Причина: cs_log_clear синхронно повертає керування після medium.erase; новий PIR обробляється тільки наступним проходом. Без ОС: idle-time maintenance, розбиття зовнішньої flash операції на стан/WIP polling. Ціна: складніший протокол журналу. ОС: пріоритетна sensor task зменшить затримку лише якщо CPU може виконувати код під час storage operation. Внутрішня single-bank flash C031 може зупиняти fetch; preemption сама це не виправляє. Гіпотеза ЛР4: на тому самому internal flash blackout не зникне; на операції, що допускає виконання CPU, пропуски повинні зменшитися. Перевіряти ці випадки окремо, не переносити висновок із fake.

## 4. Постійна історія скінченна

Симптом: 103-я подія не поміщається; старі 102 лишаються валідними після reopen. Черга також явно відхиляє дев'яту pending подію. Доказ: `tests/test_custody.c`, regression.log, full/reboot/queue assertions. Причина: фіксовані 4096 bytes, append-only recovery споживає torn slots. Без ОС: дві банківські області з поколіннями/копіюванням, зовнішній накопичувач або export перед erase. Ціна: endurance, recovery state machine, додаткова flash/RAM. ОС: producer/consumer queue спростить передачу, але не збільшить capacity і не дасть crash consistency автоматично. Ціна: пам'ять черги/стеків і необхідність синхронізації. Гіпотеза ЛР4: capacity лишиться 102, жодна підтверджена стара подія не зникне через task scheduling; повторити 41 byte-cut позицію та full test без послаблення вимог.

## 5. Вимірювальний стан і libc витрачають RAM та стек

Симптом: linker резервує 9696/12288 bytes RAM, серед великих символів RX timestamps і TX ring по 2048 bytes та два масиви perf по 576 bytes. Доказ: sections.log, symbols.log; це реальна ARM-збірка. Причина: bounded buffers, snapshots та один спільний MSP; printf має власні кадри й може використовувати newlib heap. Без ОС: компактні timestamp-черги, бінарний export, зменшення форматування; ціна складнішого протоколу. ОС: окремі стеки дадуть ізоляцію та high-water marks, але збільшать RAM; IRQ MSP теж потрібен. Гіпотеза ЛР4: сума static+heap peak+стеки всіх задач+MSP має лишитись <12288 bytes з виміряним запасом; гранично доступні 2592 bytes не можна витратити двічі як запас MSP і task RAM. У Wokwi stack_used S1/S2/S3 становив 1164/1168/1240 B із резерву 2048 B; heap_peak arena _sbrk = 0 у цих snapshots. Це не верхня межа для всіх сценаріїв і не вимір фізичної плати.

## 6. Один dispatcher пов'язує модулі й часові контракти

Симптом: додавання журналу потребувало змін event sink, start, poll, commands, linker, build та diagnostics. Доказ: збережений diff і конкретні файли runtime.c/commands.c/Makefile/linker; це огляд, не вимір часу підтримки. Причина: порядок і бюджети задач явно задає один foreground. Без ОС: таблиця callbacks/бюджетів, маленькі state machines; ціна непрямі виклики та тестування спільних deadlines. ОС: task API/queue відокремлять очікування storage від sensor task. Ціна: races, lock ordering, priority inversion і додаткова конфігурація. Гіпотеза ЛР4: додавання одного telemetry consumer потребуватиме менше змін у sensor/control коді при однакових regression tests; не підмінювати цей критерій кількістю перейменованих файлів.

Super loop зберігає один стек, прозорий порядок і просте трасування. У поточному коді немає context switching, kernel tick або RTOS object allocation. Швидкість старту, повний runtime stack і порівняльні накладні витрати ОС ще потребують вимірювання.

## Фактичні S1–S3 і межі

S1 120.490 s MCU; ADC VCD покриває лише частину. S2 3604.624 s безперервного UART за uptime, 30 GPIO NEARBY, missed IMU/PIR=1, LOG dropped=30; VCD лише до приблизно 146.6 s. S3 120.078 s MCU, GPIO 5 Hz, запропоновано 20 STATUS/s, 636 NEARBY включно зі зливом, LOG dropped=691, TX/RX dropped=0, IMU/PIR missed=1. Із 437 VCD пар максимум оцінки фронт–кінець EVENT 141.120 ms при цілі 100 ms. SysTick має крок 1 ms, кінець UART оцінено за 115200 bit/s. S3 не вимірює flash stall. Flash Wokwi має 4096 не-FF bytes, 102 damaged slots і full: збереження не підтверджено. Raw, origin та аналіз: `captures/lab3/S1-2e36-20261030/`, `S2-2e36-20261030/`, `S3-2e36-20261030/`. Ці наслідки симуляції не доводять стан фізичної flash.
