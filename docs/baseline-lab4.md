# Базова лінія CargoSeal для ЛР4

Це план повторення та поточні факти ЛР3, не реалізація RTOS. Джерело збірки `metrics/lab3-build/manifest.json`; перед порівнянням фіксувати build_id, source digest, HEX SHA і той самий diagram. Повторити піни, режими, пороги, UART 115200 8N1 та відрізки часу з ЛР3. Не міняти довжину STATUS у тесті bandwidth без окремого пояснення.

## Відтворення

У CargoSeal виконати `python3 tools/lab3_build.py`, `make test`, `python3 tools/lab3_host.py`. Зберегти original outputs. У Wokwi завантажувати саме побудований HEX через Upload Firmware, а не збирати main.c стороннього loader. При запуску акселерометр 0,0,1 g, gyro 0, LDR 0.1 lux; PIR на PA1. GPIO PA4 ADC, PA6 loop, PA7 IMU/PIR, PA8 flash. Наявні I2C PB8/PB9, UART PA2/PA3. Не використовувати PA5 як службовий marker, він показує alarm/heartbeat.

| Сценарій | Умови і тривалість | Команди і входи | Поточний результат |
|---|---|---|---|
| S1 | 120 s спокою після calibration | STATS на початку/кінці, STATUS | Wokwi 120.490 s MCU, VCD частковий; stack 1164 B, heap arena peak 0 |
| S2 | >=3600 s симуляційного часу | раз на 120 s зміна режиму і 40+ ms удар, normal >=500 ms, рідкісні STATUS, фінальні STATS/LOG STATUS/DUMP | Wokwi UART 3604.624 s MCU, 30 NEARBY від GPIO; VCD лише 146.6 s, LOG dropped=30, IMU/PIR missed=1. Host model окремо: 30 IMPACT + 30 MODE |
| S3 | 30–50 подій під час довгої операції, тисячі periodic releases | LOG DUMP/справжній LOG ERASE, STATUS 20/s, PIR, impact, LDR, потім ADDRESS 69/68 | Wokwi 120.078 s MCU, GPIO 5 Hz, STATUS 20/s; 437 VCD пар, max 141.120 ms, LOG dropped=691, IMU/PIR missed=1. Реальний flash stall не виміряно; host 40-ms hook окремо |
| UART boundary | 30 s на rate 40/50/60/80/120/200 STATUS/s | однакова відповідь STATUS, потім припинити потік, дати TX drain, STATS | model first failing tested rate 80/s, 130 dropped; 60/s без втрат; MCU boundary відкрите |
| Fault/recovery | >=1.2 s на кожній адресі | IMU ADDRESS 0x69; PIR/LDR; 0x68 | Wokwi SENSOR_FAULT / imu_valid=0 при 0x69, SENSOR_RECOVERED / imu_valid=1 при 0x68; фізична SDA і timeouts не випробувані |
| Startup | запис від нуля | ADC/IMU/PIR ready, calibration time | Wokwi banner/selftest записані; фізичний startup не виміряний |
| Power loss | перед/після partial write | LOG DUMP, CRC і boot, повтор на різних позиціях | 41 byte-cut host checks; реальний brownout/reset retention відкрите |

При 30 подіях зберегти min/avg/max/p99 окремо для PIR edge→dispatch, PIR edge→повний EVENT у UART і CR/LF→повна відповідь. `lf_dispatch` не включає TX backlog і час передачі. Вимір timer проти GPIO ADC робити від одного reset до того самого STATS snapshot. Capture loop короткий; для тривалих captures не під'єднувати PA6 до analyzer, інакше buffer витрачається на dispatcher. Зміни тільки wiring analyzer фіксувати в origin manifest.

## Таблиця для однакового повторення

| Метрика | Метод | Підтверджений рівень зараз |
|---|---|---|
| Code flash | ARM linker + map | 27388 B /28672 B, ще 4096 B журнал |
| RAM static з вирівнюванням | size -A .data +.bss | 7132 B, плюс 2564 B section heap/stack; разом 9696 B |
| Stack | 2048 B painting + .su audit і production disassembly | Wokwi S1/S2/S3: 1164/1168/1240 B, не гарантія всіх шляхів; audit без LTO не є production bound |
| Heap | `_sbrk` peak | Wokwi _sbrk arena peak 0 у S1/S2/S3; fake нуль не є виміром |
| Durations/loop/jitter/missed/latencies | SysTick 1 ms, GPIO/VCD, wire UART | S1/S2/S3 записані з обмеженнями coverage; S3 max 141.120 ms до оціненого кінця EVENT |
| Врахований foreground load | сума активних блоків / window | Wokwi snapshots S2 148 permille, S3 167 permille; не повна CPU utilization |
| Log consistency | CRC + commit + full/torn tests | host C tests пройшли, MCU retention відкрите |

Метрики після доповнення збірки брати з актуального manifest/sections, а не з застарілої таблиці. Гіпотези, механізми ОС і ціна кожного викладені в limitations.md. Не приписувати ОС усунення UART bitrate, capacity, flash fetch stall або aliasing 50-Hz датчика. Позитивні властивості super loop для повторення: startup, один MSP, code/RAM size, порядок подій і відсутність task-level deadlocks.

## Збережені результати та обмеження

ELF SHA-256 `4ed0f50d97b74b8b4ba2bb74a5432d72f866ca0757d987942e3dc1519fe7cb99`, digest і build id у manifest. Raw, VCD, origin, result і summaries збережено в `captures/lab3/S1-2e36-20261030/`, `S2-2e36-20261030/`, `S3-2e36-20261030/`; метод і межі наведено у звіті. GPIO-генератор не є фізичним PIR. S2 VCD не незалежний годинний запис. У Wokwi flash 4096 не-FF байтів, 102 damaged slots, full; запис і збереження не підтверджено. Для порівняння ОС зберегти ці відмовні результати, а фізичну flash, brownout і час реального стирання перевірити окремо.

Додаткова точка порівняння: 321 ADC release S3, STAT min/avg/max 0/183/1000 us та ADC_MARK 89.175/104.724/774.175 us, різні межі вимірювання і крок SysTick 1 ms. Коротка перевірка SDA LOW від старту до 47.093 s дала valid=0, після release отримано SENSOR_RECOVERED; UART після 58 s: 36 пар STATUS, max 18.495 ms до оціненого кінця, під час попереднього перевантаження tx_dropped=61 rx_dropped=194. Повторити з однаковим input та вікном; це не доводить роботу фізичної плати або flash.
