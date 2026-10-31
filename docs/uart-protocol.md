# UART CargoSeal

115200 біт/с, 8N1. Кожна команда завершується CR, LF або CRLF. У Wokwi Terminal введіть команду та натисніть Enter. CRLF виконує команду один раз. Регістр і пробіли точні. Понад 63 символи, NUL або невідома команда дають ERR COMMAND після завершення рядка. Незавершена команда не виконується.

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

# Діагностика capability pilot ЛР3

LOG STATUS та відповідь LOG ERASE містять MEDIUM checked/non_ff/first/first_value. checked=4096 і non_ff=0 означають повний all-FF readback; це не гарантія подальшого program або retention. BOOT і METRICS повідомляють clock_resolution_us=1000 для SysTick. Поля з суфіксом us мають роздільність 1000 us, а не 1 us. Для коротких інтервалів використовувати GPIO/VCD; пропущені під час flash-блокування SysTick не відновлюються. Незалежне порівняння зі шкалою VCD обов'язкове.

