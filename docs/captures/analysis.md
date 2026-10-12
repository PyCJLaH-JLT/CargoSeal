# Аналіз власних записів CargoSeal

Записи зроблено 13 жовтня 2026 у Wokwi з HEX цього репозиторію. Це форма B, фізичних вимірювань немає. Онлайн приклад ST Nucleo HAL використано лише як тимчасовий завантажувач власного HEX і diagram.json, без Save/Fork. main.c прикладу не виконувався.

Повні VCD збережено як final-initialization.vcd.gz та final-acceptance.vcd.gz. manifest.json містить SHA256 HEX/ELF/BIN, вихідних VCD та призначення семи каналів. Кожен *.origin.json задає hash джерела та початкове зміщення фрагмента. Час у PulseView відносний до початку фрагмента.

Аналізатор Wokwi записує зміни рівнів з timescale 1 ns, trigger off, buffer 4 000 000 переходів. Це не фізичний аналізатор 1 GHz. Для .sr I²C/UART дані ресемпловано до 20 MHz, для GPIO до 1 MHz. Останній рівень при декількох переходах в один timestamp є рівнем після обробки цієї часової мітки. Це звірено з sigrok і захищено regression test.

| Файл VCD/.sr | Канали | Зміщення у повному джерелі | Як повторити |
|---|---|---|---|
| i2c-mpu6050-id | SCL, SDA | 4.4..5.25 ms, initialization | Перезапуск або READ IMU; WHO_AM_I |
| i2c-mpu6050-burst | SCL, SDA | 9.1..11.15 ms, initialization | READ IMU або штатний poll 20 ms |
| i2c-mpu6050-nack | SCL, SDA | 15093.0..15093.5 ms, acceptance | IMU ADDRESS 0x69 за AD0 LOW |
| uart-read-imu | UART_TX | 548..573 ms, initialization | READ IMU |
| gpio-pir-adc | PIR, ADC_MARK, LED | 6900..12020 ms, acceptance | Simulate motion після LOW, ADC працює постійно |

## I²C

На PB8/PB9 виміряно T=10.000 µs, high=5.000 µs, low=5.000 µs, f=100 kHz. За TIMINGR 0x20303E5D, PCLK=48 MHz, PRESC=2 маємо tPRESC=62.5 ns, high=63·62.5=3.9375 µs, low=94·62.5=5.875 µs. Сума 9.8125 µs без синхронізації та фільтрів. Модель Wokwi дає період на 1.91% довший та симетричну форму; це не доказ фізичних rise/fall time.

Власний Board/imu_bus.c використовує I2C1 AUTOEND, окремий STOP після запису register pointer та наступний START читання. MPU6050 зберігає pointer, що підтверджено читанням ID і всіх 14 байтів. HAL Mem_Read очікував TC, якого цей запуск моделі не давав, і завершувався timeout після ACK адреси/регістру. Guard 120 µs після TXDR та завершення STOP дає моделі закінчити передачу перед наступною фазою. Дані RX читаються за RXNE без штучного guard. NACK, BERR/ARLO/OVR і deadline 10 ms обмежують кожен виклик. Переносимий Sensors не знає цієї адаптації.

ID, повна ручна розшифровка:

```text
START
11010000 = D0 = address 1101000 (0x68), R/W=0
ACK=0, дев'ятий SCL
01110101 = 75, WHO_AM_I pointer, ACK=0
STOP
START
11010001 = D1 = address 0x68, R/W=1, ACK=0
01101000 = 68, WHO_AM_I
NACK=1 від master, кінець одно-байтового читання
STOP
```

За InvenSense RM-MPU-6000A-00 Rev 4.0, с. 46, ID у 0x75 дорівнює 0x68. Master NACK після останнього байта означає завершення читання. Address NACK на D2, тобто відсутньому 0x69 за AD0 LOW, є помилкою.

Ініціалізація записує 6B=01, 1A=03, 1B=00, 1C=10, 19=09, потім читає діапазони 00 10. За картою регістрів, с. 14–15, це ±250 °/s та ±8 g. Burst від 0x3B:

```text
00 00 00 00 10 00 EF 5C 00 00 00 00 00 00
ax=0/4096=0 g; ay=0 g; az=0x1000/4096=1 g
temp=sign16(EF5C)/340+36.53=24.0006 °C
gx=gy=gz=0/131=0 °/s
```

Read phase займає 1365 µs. Розрахунок 15 байтів включно з адресою, 15·9·10 µs=1350 µs плюс START/STOP 15 µs. ACK на проміжних байтах і NACK на останньому відповідають завершенню 14-byte read. Саме цей frame використано в tests/fixtures/mpu6050_observed.h. Відповідність driver output перевіряється автоматично.

## UART

USART2 PA2/PA3, 115200, 8N1. Розрахунок біта 1/115200=8.68056 µs, байта 10 біт=86.8056 µs. У VCD типовий біт 8.675..8.700 µs. В uart-read-imu.annotations.txt незалежний sigrok decoder відновлює READ_IMU, ID і той самий raw frame, CRLF.

Перші ASCII байти READ_IMU: 52 45 41 44 5F 49 4D 55. Наприклад R=0x52 передається LSB first 0,1,0,0,1,0,1,0, між start=0 і stop=1. Це UART TTL рівнів 3.3 V, не RS-232.

У final-acceptance є seq 1..36 без пропусків, один ready і послідовні tick. За 31 командою MODE/STATUS/BAD повний відповідний рядок UART, включно зі stop bit LF, завершився через 1.497..36.643 ms після stop bit прийнятого LF. В усіх STATUS tx_dropped=0, rx_dropped=0. Черги UART передають interrupt mode, не блокують основний цикл довгим printf.

20 STATUS/s перевірено лише у timed fake UART runtime test. Окремий попередній Wokwi run містить 20 послідовних STATUS приблизно 3/s, обмежених швидкістю UI; це не симуляційне підтвердження навантаження 20/s.

## GPIO та ADC

PIR PA1 створює фронт HIGH. Дві повні HIGH-ділянки VCD мають ширину 5000.000025 ms, відповідно до delayTime=5 у diagram.json. Один NEARBY на кожну ділянку у INSTALLED, без повторів. Час від фізичного фронту моделі до повного рядка першого NEARBY 26.152 ms, наступного за fault 17.448 ms. При переході TRANSPORT → INSTALLED з уже HIGH подія прив'язана до команди переходу, а не до старого фронту; повтор MODE INSTALLED не створює події.

ADC цифрової шини не має. PA4 HIGH безпосередньо перед HAL_ADC_Start, LOW після HAL_ADC_Stop. Медіанний період MARK=49.99745 ms, межі 46.91878..53.08915 ms через I²C у головному циклі. Медіанна ширина MARK=71.675 µs охоплює HAL Start/Poll/GetValue/Stop і виконання коду, не лише перетворення. Розрахунок sampling+conversion за 12 MHz та 160.5+12.5 cycles становить 14.42 µs без overhead.

В темряві 0.1 lux моделі raw=4063, E=32. Після 20 відліків на time_ms=961 baseline E0=32. Це ADC-коди, не виміряні люкси. При максимальному світлі моделі raw=32, E=4063, delta=4031. Порогове середнє з чотирьох відліків та таймер 300 ms створюють LIGHT_CHANGE, зворотний поріг 400 і 500 ms дозволяють повтор.

## Приймання й межі доказів

T01..T12 перевірено host tests і власними записами. У фінальному run MOVE має alarm=0, DISTURBANCE alarm=1; ax=2,az=2 дає d=1.828 g та IMPACT в обох режимах. Межі az=2.49/2.50/2.51 g перевірено host tests, бо ползунки Wokwi обмежені ±2 g по осі. Точні 599/600/601 та 400/401 ADC-коди також задаються в host tests, а не в lux-ползунку.

Одночасні входи підготовлено під час Pause, потім Resume. PIR edge=6906.6367 ms, перший ударний read закінчується 6917.550575 ms, MARK першого світлового відліку=6913.24005 ms. Окремі UART записи seq 5 NEARBY, 6 IMPACT, 7 LIGHT_CHANGE мають time_ms=6911,6915,7213. Усі три збережені без дублю DISTURBANCE. Указані time_ms є HAL timestamp, а час wire у таблицях і UART-файлах є VCD timestamp.

SENSOR_FAULT при 0x69 не зупиняє ADC та PIR. Після 0x68 отримано SENSOR_RECOVERED; ACK при invalid дає ERR ACTIVE, після нормалізації OK ACK. Alarm та E0 зберігаються при MODE. LED у тривозі HIGH; в нормі перемикається раз на 1 s.

Короткі імпульси, точні timer boundaries, invalid ADC та UINT32 wrap перевірено на хості. Форма B не підтверджує струми, RC фронти, аналоговий шум, фізичну дальність PIR, працездатність breakout або чутливість до реальних транспортних ударів.

## Повторне декодування

```sh
python3 tools/analyze_capture.py
sigrok-cli -i docs/captures/i2c-mpu6050-id.sr -P i2c:scl=SCL:sda=SDA --protocol-decoder-annotations i2c=addr-data
sigrok-cli -i docs/captures/uart-read-imu.sr -P uart:tx=UART_TX:baudrate=115200:format=ascii:tx_packet_delim=10 --protocol-decoder-annotations uart=tx-packets
```

PulseView: Open .sr, Add protocol decoder I²C з SCL/SDA або UART з TX=UART_TX, 115200, 8N1. Для GPIO відкрийте gpio-pir-adc.sr без протокольного decoder. Cursors вимірюють інтервали; незмінений *.vcd.gz дає повний run для незалежної перевірки.

Додаткові PulseView Timing кадри: i2c-clock.sr має Data=SCL, по5.000 µs наHIGH/LOW. adc-marker-width.sr має Data=ADC_MARK,71.650 µs при20 MHz. У periodview49.926 msLOW+0.071675 msHIGH=49.997675 mscycle; any-edge Average не є частотоюопитування. Канали перейменовано лише для автоматичного Data assignment, рівні й timestamps не змінено. uart-read-prefix.sr містить читабельний початок повногоREAD_IMU.
