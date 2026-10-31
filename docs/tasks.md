# Задачі CargoSeal у super loop

| Задача | Запуск | Дедлайн / критерій | FR | Реалізація |
|---|---|---|---|---|
| PIR та MPU6050 | 20 ms, пропущені слоти не наздоганяються | подія до 100 ms після фронту/підтвердження; IMU timeout одного bus call 10 ms | 04–07, 10–12 | Board/runtime, Sensors, Policy/container |
| LDR та світлова політика | 50 ms | 20 калібрувальних samples, підтвердження 300 ms; запис до 100 ms після нього | 08, 12 | adc_read, cs_light_measure |
| Команди | RX ISR, не більше 64 bytes за прохід | повна відповідь MODE/STATUS до 100 ms після CR/LF; lf_dispatch вимірює лише ISR-delimiter до dispatch | 03, 13, 14 | rx_ring, line reader, command |
| UART TX | подія, IT blocks до 128 bytes | backlog обмежений 2047 bytes, втрати видно у STATUS/STATS | 01, 02, 09, 14 | transmit, TxCpltCallback |
| Постійний журнал | черга до 8 подій, один doubleword за прохід | commit підтверджується тільки після 5 записів і readback; ціль до 100 ms без backlog, перевірка MCU відкрита | 15–17 | custody_log, log_medium |
| Стирання журналу | явна LOG ERASE, коли pending=0 або storage failed | дві сторінки, блокувальний HAL, виміряти на тому самому стенді | 17 | command, HAL_FLASHEx_Erase |
| Знімок статистики | STATS, один task row за прохід | зберігає n/min/sum/max/jitter/late/missed/deadline; вивід має власну вартість | 18 | diagnostics_output |
| Читання журналу | LOG DUMP, один slot за прохід, backpressure | не займає цикл блокувальним UART; не є атомарним snapshot | 16 | diagnostics_output |
| Heartbeat і LED | 1000 ms / кожен прохід | alarm утримує LED HIGH, інакше toggle раз на секунду | 02, 13 | runtime_poll |

Порядок: TX service, IMU/PIR, ADC, до 64 RX bytes, один flash doubleword, diagnostics, heartbeat/LED. ISR не викликає прикладні правила або printf. Час бере значення HAL SysTick з кроком 1 ms; поле us є перерахунком і не додає точності. Різниця усередині runtime_ms коректна через один 32-bit wrap значення us за умови викликів частіше ніж 71.58 min. Якщо flash блокує обробку SysTick, пропущені ticks не відновлюються. Реальну затримку коротких операцій слід визначати за GPIO/VCD; власний Wokwi gate і часткові S1/S2/S3 captures зафіксовано у captures/lab3; фізична плата і flash stall не перевірені.

Спільні змінні ISR/main: rx_head/rx_tail, tx_busy і HAL tick. У producer/consumer єдиний writer кожного індексу; byte/timestamp записуються перед публікацією rx_head. Читання часу не змінює спільний стан і використовує один 32-bit HAL tick. На Cortex-M0+ aligned 32-bit доступ атомарний, але volatile не робить пару даних атомарною. DMA у цій Wokwi-моделі не реалізована, RTOS немає.

Усі STAT duration/jitter і accounted LOAD квантуються до 1 ms. Багато коротких операцій можуть давати нуль навіть при ненульовій реальній вартості, а сума LOAD може бути зміщеною. Нуль не є доказом відсутності витрат. Для коротких операцій потрібні GPIO/VCD; LOAD не вимірює повну CPU utilization.

CPU LOAD означає частку часу, враховану роботою foreground, а не повну утилізацію CPU. Busy spin, ISR, частина dispatcher та instrumentation overhead не входять. Без WFI ядро не спить, навіть коли ця метрика мала. Відлік loop охоплює виклик runtime_poll, loop_period окремо вимірює відстань між входами, включно з main/call overhead. GPIO marker додає час виміру. Потрібні незалежні записи і калібрування overhead. RX producer публікує head після DMB, consumer читає byte/timestamp після acquire barrier і публікує tail після копіювання; volatile саме по собі не встановлює порядок цих non-volatile даних.
