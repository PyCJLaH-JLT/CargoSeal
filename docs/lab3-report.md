# CargoSeal: технічний стан ЛР3

Bare-metal super loop зберігає правила TRANSPORT/INSTALLED, руху, удару та світлової ознаки. Реалізовано bounded custody journal з CRC, commit та readback. RTOS не додано.

ARM-збірка `90392f53-2e36b428364c`: 27388/28672 B code flash, 4096 B окремо зарезервовано для журналу, RAM з резервами 9696/12288 B. Версію джерел та ELF/HEX фіксує [manifest](metrics/lab3-build/manifest.json). Результати host-тестів не є виміром MCU.

S1 тривав 120.490 s MCU. S2 має безперервний UART за 3604.624 s MCU і 30 GPIO NEARBY, але VCD охоплює лише приблизно 146.6 s; LOG dropped=30, IMU/PIR missed=1. S3 тривав 120.078 s MCU з GPIO 5 Hz і запропонованими 20 STATUS/s: 437 VCD пар, оцінений максимум до кінця EVENT 141.120 ms, понад ціль 100 ms; LOG dropped=691, IMU/PIR missed=1. GPIO не є фізичним PIR, а S3 не вимірює flash stall. Архіви, умови, точні хеші та обмеження наведено у [captures](captures/lab3/README.md).

SysTick має крок 1 ms, а пропущені при блокуванні ticks не відновлюються. Stack painting S1/S2/S3 показав 1164/1168/1240 B; heap arena peak був 0 у snapshots, це не гарантія усіх шляхів. Accounted foreground LOAD не є повною CPU utilization. Flash Wokwi повернула 4096 non-FF bytes, 102 damaged slots і full; збереження, фізична плата і brownout не підтверджені. Тести, факти і непідтверджені FR розділено у [звіті PDF](lab3-report.pdf).

[Задачі](tasks.md), [метрики](metrics/README.md), [обмеження](limitations.md), [журнал проблем](lab3-problems.md), [база ЛР4](baseline-lab4.md). Команди: `python3 tools/lab3_build.py`, `make test`, `python3 tools/lab3_host.py`.

Додаткова траса startup та SDA: підключений MPU, SDA LOW до 47.093 s, valid=0, після release SENSOR_RECOVERED. За 36 окремими UART парами STATUS max 18.495 ms до оціненого кінця; під перевантаженням TX/RX dropped=61/194. ADC на спільному вікні 321 виклику: GPIO avg 104.724 us, SysTick avg 183 us при кроці 1 ms та різних межах коду. [Докази й метод](captures/lab3/README.md).

У [короткому Wokwi-тесті Flash](captures/lab3/flash-20261101/README.md) того самого ELF команда LOG ERASE повернула ok=0, STORAGE_FAULT; наступний scan: 4096 non-FF bytes. Точний статус HAL та повний дамп після виконання не отримано; причина відмови невідома. FLASH_MARK у VCD тривав 13.478 ms під debugger, що не є виміром успішного стирання або фізичної flash.
