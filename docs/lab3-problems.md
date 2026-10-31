# Проблеми ЛР3 та виправлення

Історичний pilot на старому TIM3 ELF виявив три питання; нижче наведено його факти, а підсумок нового SysTick ELF окремо.

- BOOT показав build=unversioned при правильному SHA HEX. Причина: make -B повторно виконував recipe build-id.txt. Recipe зберігає наявний файл, а lab3_build.py перевіряє ID всередині ELF. Новий ARM build пройшов; MCU BOOT ще перевіряється.
- Tick зріс з 8 до 4303 між 8.032 s та 8.328 s VCD. Причину зворотного timestamps ще не встановлено. CLOCK_FAULT зберігає перший register snapshot; tick пропускає пропущені інтервали замість catch-up flood. Recorder відхиляє CLOCK_FAULT і стрибок STATUS понад 60 s. Це діагностика, а не доведене виправлення таймера.
- Startup flash містив 102 damaged slots. Сирі bytes і наслідок erase у цьому запуску не виміряні. MEDIUM перевіряє всі 4096 bytes; erase відхиляється при non-FF навіть у хвостах сторінок. Unit tests перевіряють offsets 2047 і 4095; MCU readback ще потрібен.

| Симптом | Причина | Виправлення та доказ |
|---|---|---|
| Linker overflow 1208 bytes при 28 KiB code region | 4 KiB віддано журналу, попередні правила та форматування лишили великий код | -Os/-flto, без зміни MCU; Lab03/Work/build-first.log, актуальний build.log та Makefile |
| LTO link: undefined reference `_sbrk` | low-level libc hook не збережено до пізнього newlib reference | used/externally_visible у Core/Src/sysmem.c; build-lto.log і build-lto-fixed.log у локальному Work, успішний production link |
| Cut-test очікував invalid після кожного неповного запису | останні три bytes commit для seq=18 вже дорівнюють erased FF; після 37 bytes результат повністю дорівнює валідному запису | критерій приймає цілісний payload/CRC/commit, а не кількість викликів byte writer; test_custody.c перевіряє 41 позицію і збереження старого запису |
| STATS після перевантаження не містив MEMORY row у host rate 80/120/200 | bounded TX була повна; сам запит статистики втратився серед відповідей | тест припиняє потік, чекає 2 s drain, потім питає STATS; runtime продовжує чесно рахувати tx_dropped. Вивід діагностики під безмежним навантаженням не гарантовано |
| .su поруч із production LTO могли бути старими файлами попередньої збірки | GCC10 LTO не видав новий побайтовий per-function audit у тих самих .su; збережені попередні артефакти вводили в оману | окремий stack-audit без LTO, production disassembly з актуального ELF; методи підписано окремо у metrics/README.md |

Історичний TIM3-кандидат використовував runtime_ms від delta TIM3. Фінальний scheduler використовує HAL SysTick з кроком 1 ms. SysTick ticks можуть зливатися при flash stall, тому HAL_GetTick не слід вважати незалежним wall time для scheduler/latency через erase. Це виправлення з огляду коду, а не підтверджена фізична аварія. У runtime LOAD усунуто подвійний облік erase всередині command; показник усе одно є foreground accounted time, без ISR та spin.

Версія кожного файлу задається source_sha256 у metrics/lab3-build/manifest.json. Реальні логи, captures та дати не переписуються під бажаний результат. Для проблеми cut-test початковий assertion спостерігався в інструментальному виводі; збережений тест і повторний regression log підтверджують виправлений критерій, але не є первинним логом падіння.

## Підсумок підтвердженого ELF

На збірці `90392f53-2e36b428364c` BOOT і source digest підтверджені; короткий VCD gate дав 99.000877 s між tick 1 та 100. S1/S2/S3 записано окремо, S2 UART охоплює 3604.624 s MCU, VCD тільки близько 146.6 s. Flash Wokwi повернула 4096 non-FF bytes, valid=0, damaged=102, full=1; запис і збереження не підтверджено. Висновок про фізичний flash stall не випливає з S3.

Код та обладнання: commit `b5568186de2894fc01fc00ff0769c65d337893c8`, регресійні тести й інструменти: `eb5f52e6a0a0e5391346566221790685033f31e8`. Це коміти джерел після вимірювання; ідентичність виміряної збірки задає manifest, а не новий HEAD.

Архів фактичних захоплень і результатів host-моделі: `25268c70ff40cdecbc052841d799ec11663a455b`.
