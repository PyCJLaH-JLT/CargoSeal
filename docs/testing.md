# Перевірки CargoSeal

Виконано 2026-10-13 на macOS arm64/clang і Linux aarch64/GCC у власному Docker-образі cargoseal-build:lab2. Шість груп host tests пройшли. Переносимі Policy/Sensors та Board runtime виконуються з контрольованим часом і fake HAL. Це програмна перевірка, а не фізичне випробування або запуск Wokwi.

| Група | Що перевіряє |
|---|---|
| sensors | WHO_AM_I, порядок п'яти записів, readback ±8 g/±250 °/с, signed big-endian, 4096 LSB/g та 131 LSB/(°/с), NACK/timeout на кожному кроці, невалідні ID/ranges, незмінність output після помилки |
| light | raw 0/4095/4096, E=4095-raw, rolling mean чотирьох вимірів, baseline із 20 raw E, сталість baseline, PIR edge |
| policy | 200 мс руху, пріоритет удару, один запис епізоду, fault/recovery, незалежні PIR/ADC та seq |
| commands | Два явні режими, HIGH PIR при вході, повтор MODE, збереження alarm, ACK guard, LF/CRLF, NUL і довга команда |
| acceptance | 2.49/2.50/2.51 g; 4.99/5 °/с; світло 599/600/601 та hysteresis 400/401; 199/200, 299/300, 499/500 мс; короткий імпульс; simultaneous PIR/IMPACT/LIGHT_CHANGE; очищення timer при MODE; invalid ADC; UINT32 wrap |
| runtime | Штатні 20/50 мс; startup/20 samples; MODE лише після LF; ACK до/після норми; помилкова адреса та відновлення; PIR/ADC за недоступного IMU; 20 STATUS/s з UART-моделлю 115200, без queue loss |

AddressSanitizer/UndefinedBehaviorSanitizer пройшли для всіх шести груп без діагностик. Fake UART передає асинхронно з тривалістю 10 біт/байт; він перевіряє черги та scheduler, але не USART регістри моделі Wokwi.

GCC coverage для однієї instrumented copy переносимих модулів:

| Модуль | Виконані рядки | Branch directions taken |
|---|---|---|
| Sensors/mpu6050.c | 28/28, 100% | 22/22, 100% |
| Sensors/light.c | 17/17, 100% | 12/12, 100% |
| Policy/container.c | 83/83, 100% | 65/68, 95.59% |
| Policy/commands.c | 14/14, 100% | 17/18, 94.44% |

Ціль ≥90% рядків і ≥80% напрямків гілок обрана для переносимого коду з перевіркою негативних сценаріїв. Відсоток не замінює вимоги: окремо перевіряються числові границі й переходи. Core, Board та Drivers не входять до цього coverage, апаратну інтеграцію додатково перевірено Wokwi та записами сигналів, фізичну плату не перевіряли.

Команди: make test; make coverage. Для sanitizer: make -C tests clean; make -C tests test CFLAGS='-std=c11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer'. Кожна платформа має власний tests/build/OS-arch. CI запускає build/test/coverage на push і pull_request до main.

FR-01/02 перевіряються startup і heartbeat у runtime та потребують запису UART/LED. FR-03/10/13/14 покрито commands/runtime; FR-04/05/06 acceptance; FR-07 light/commands/runtime; FR-08 light/acceptance; FR-09 policy та подальшим декодуванням журналу; FR-11 sensors/runtime; FR-12 acceptance/runtime. У final-acceptance.vcd збережено 36 подій; повний MODE/STATUS/ERR після LF займає 1.50..36.65 мс. Час визначено за UART RX/TX з урахуванням stop bit LF. Деталі у docs/captures/analysis.md.

Реальні 14 байт MPU6050 з першого burst фінального VCD додано до tests/fixtures/mpu6050_observed.h та перевірено driver test. Два Python regression tests захищають порядок переходів з однаковою часовою міткою і звіряють WHO_AM_I з власного запису. sigrok-cli незалежно підтверджує 0x68 і raw frame.
