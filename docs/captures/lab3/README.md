# Записи нової прошивки ЛР3

Власний короткий MCU/Wokwi pilot збережено в pilot-original/: результат негативний, часові метрики недостовірні. Між 8.032414525 s та 8.327614125 s VCD UART tick змінився з 8 на 4303. BOOT показав build=unversioned, хоча SHA завантаженого HEX збігся з manifest. Startup log: valid=0, damaged=102, full=1; сирі bytes flash та erase цим запуском не встановлено. Це не S1/S2/S3 і не годинний тест. Записи ЛР2 збережено під початковими іменами.

Для кожного нового запуску створити окреме ім'я S1/S2/S3/date, зберегти оригінальний VCD/serial bytes та origin.json: UTC, build_id, SHA HEX/ELF, SHA exact diagram, commands, час MCU початку/кінця, умови sensor/fixture та всі pauses. Копію current-run.vcd переносити в окремий run до наступного запуску.

`tools/lab3_capture.py` декодує тільки існуючий VCD, не генерує сигналів. Для `--pir-installed-only` потрібен відрізок без MODE transitions, що самі можуть створити NEARBY. UART RX у спеціальному fixture займає D0 замість SCL. Стандартна схема D0=SCL, D1=SDA, D2=UART_TX, D3=PIR, D4=ADC_MARK, D5=IMU_MARK, D6=FLASH_MARK, D7=LOOP_MARK.

Сценарні схеми в tools/scenarios мають ту саму прошивку. S1/loop використовують справжню модель PIR. S2/S3/uart мають явно позначений цифровий clock stimulus замість PIR OUT; це перевірка обробки GPIO фронтів, не фізичного PIR. S2 1/120 Hz, S3/uart 5 Hz. При довгому capture D7 від'єднано, щоб fast loop не заповнив buffer. Жоден такий stimulus diagram не є захопленням або доказом виконання сам по собі.

## S1/S2/S3, 30.10.2026

Каталоги `S1-2e36-20261030`, `S2-2e36-20261030`, `S3-2e36-20261030` містять власні UART bytes, commands, origin, result і gzip-архів оригінального VCD. Геші розпакованих VCD: S1 `449380e63f4a7b9b9b1a40ea5f1c8601602354d2977148a5e9b11a1d779c6552`, S2 `febfab436b0c8ee8db9393104074a5d0ba5fb014f14ec8c9d7d2bc319eacf1d6`, S3 `b1ffe6d05d2cf61e2b62988eef6c36461c5528ebef0dbdb3b7002fc4d96730a6`. ELF `4ed0f50d97b74b8b4ba2bb74a5432d72f866ca0757d987942e3dc1519fe7cb99`, build `90392f53-2e36b428364c`. Сценарій S1 записано після окремого fault test, статистики накопичені. S2 UART 3604.624 s MCU, але VCD до приблизно 146.6 s; S3 120.078 s MCU, GPIO 5 Hz, запропоновано STATUS 20/s, VCD частковий. Flash full/damaged: жоден із записів не доводить flash stall чи persistence.
