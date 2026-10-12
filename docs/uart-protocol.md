# UART CargoSeal

115200 біт/с, 8N1, LF після кожної команди. CRLF також приймається. Регістр і пробіли точні. Понад 63 символи, NUL або невідома команда дають ERR COMMAND після LF. Незавершена команда не виконується.

| Команда | Дія |
|---|---|
| STATUS | Режим, alarm, IMU valid, ADC raw, baseline ready, uptime_ms та counters черг |
| MODE TRANSPORT | Явний перехід у перевезення |
| MODE INSTALLED | Явний перехід у встановлений стан |
| ACK | Скидання alarm після нормалізації IMU і світла 500 мс; інакше ERR ACTIVE |
| SELFTEST | Зведення поточного стану сенсорів; PIR GPIO readable не доводить фізичну справність |
| READ IMU | Повторюване читання WHO_AM_I та 14 raw bytes для аналізатора |
| IMU ADDRESS 0x69 | Навмисна адресна помилка, якщо AD0 LOW; штатний fault/retry шлях |
| IMU ADDRESS 0x68 | Відновлення правильної адреси та повторна ініціалізація |

Діагностика не змінює режим, baseline або alarm. B1 не перемикає режими і не скидає alarm. Потік подій містить seq, time_ms, mode, event, source, значення з одиницями й valid. MOVE та NEARBY інформаційні; DISTURBANCE, IMPACT, LIGHT_CHANGE встановлюють alarm. LIGHT_CHANGE означає можливий доступ.

