# Перевірка збірки CargoSeal

Фактичні запуски виконано 2026-10-04. Час у логах UTC. Дати комітів не є датами виконання цих перевірок.

## Запуски

| Запуск | Початок UTC | Середовище | Результат |
|---|---|---|---|
| [Mac 1](mac-build-1.log) | 09:28:16 | Docker 29.6.1, образ arm64 | `make clean`, `make build`, exit 0 |
| [Mac 2](mac-build-2.log) | 09:28:20 | Docker 29.6.1, образ arm64 | `make clean`, `make build`, exit 0 |
| [Ubuntu](ubuntu-build.log) | 09:36:04 | Ubuntu 24.04.5 LTS, Docker 29.1.3, образ amd64 | `make -s clean`, `make -s build`, exit 0 |

Прапорець `-s` прибирає друк рецептів Makefile, не змінює чистку чи компіляцію. В Ubuntu-лозі службове позначення `<artifact-mount>` приховує абсолютний шлях монтування, вивід інструментів збережено. GCC у всіх запусках `10.3.1 20210621`, пакет `15:10.3-2021.07-4`. Dockerfile однаковий; образи різних архітектур і метадані їх збірки не зобов'язані мати однакові image ID. Повний rebuild firmware виконано після кожного clean, Docker-кеш використано для тулчейну.

## SHA256

Окремі списки: [Mac 1](mac-build-1.sha256), [Mac 2](mac-build-2.sha256), [Ubuntu](ubuntu-build.sha256).

| Артефакт | Mac 1 і Mac 2 | Ubuntu |
|---|---|---|
| ELF | `0f7b87f63097adfc290c57ea018a65e5a05da061aaa48fd520c14fbfe05f85da` | `0f9a6dd253dafdc57692ef229aaa3e994e70949ead8f82103967e4e85efa1ff7` |
| HEX | `20c68952fd3c7ab6640f8ca53935729d6ca2c54d55dbba98597a92c2e0248415` | Те саме |
| BIN | `c21e312881deefbec24a06e9808d3651367530ac8ea930d3b37cc011daa7f7df` | Те саме |

Дві Mac-збірки побайтно однакові для всіх трьох артефактів. Між платформами HEX і BIN однакові, вихідні ELF різні. Це обмеження повної відтворюваності ELF, яке не приховується порівнянням тільки BIN.

## Дослідження ELF

[elf-analysis.log](elf-analysis.log) містить ELF headers, перелік секцій, порівняння їх вмісту та результат `arm-none-eabi-objcopy --strip-debug` на окремих діагностичних копіях. Початкові артефакти не змінювалися.

Виявлено одну різницю вмісту секцій, рядок `.debug_str` із каталогом збірки libgcc:

```text
Mac:    /build/gcc-arm-none-eabi-A9GHXF/gcc-arm-none-eabi-10.3-2021.07/build/arm-none-eabi/thumb/v6-m/nofp/libgcc
Ubuntu: /build/gcc-arm-none-eabi-hYfgK4/gcc-arm-none-eabi-10.3-2021.07/build/arm-none-eabi/thumb/v6-m/nofp/libgcc
```

Усі секції з прапорцем ALLOC мають однакові байти, адреси й розміри. Після видалення debug-секцій обидві діагностичні копії ELF мають SHA256 `a3e7c7f6f75dad9b378868472978f3557b3f3ae74a45a7e1d531e48d711859de` і побайтно збігаються. Висновок про причину різниці спирається на порівняння `.debug_str`, а не лише на архітектури хостів.

ELF має ELF32, little-endian, ARM, EABI5, soft-float ABI, entry point `0x08003271`. `arm-none-eabi-size` показав text 13464, data 120, bss 1808 байтів у кожній збірці. У прошивці виділено Flash 32 KiB і RAM 12 KiB. `size` включає у bss також резерв heap/stack, тому ці числа не є виміром максимального використання RAM під час роботи.

## Що перевірка не підтверджує

Код шаблону переглянуто, firmware зібрано й артефакти порівняно. Wokwi не запускався. Ready/tick, LED, значення частоти I²C та будь-які тести сенсорів під час виконання ще не підтверджено. Майбутні вимоги ЛР2 у PRD не реалізовані. Скріншоти й PDF-звіт тут відсутні.

Для повтору достатньо виконати `make clean`, `make build` і SHA256 усіх трьох файлів. На Ubuntu команда `sha256sum firmware/build/firmware.elf firmware/build/firmware.hex firmware/build/firmware.bin`; на macOS відповідник `shasum -a 256` з тим самим списком.
