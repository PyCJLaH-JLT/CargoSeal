# CargoSeal
Контроль контейнера з експонатом. Літвінов Руслан Олексійович, ІО-36. Nucleo-C031C6, STM32 HAL, Docker та Wokwi, форма B.

ЛР2 продовжує злиту ЛР1 у цьому репозиторії. MPU6050 читає прискорення й обертання через I²C, PIR формує GPIO-фронт, LDR використовує ADC PA0. Оператор явно обирає TRANSPORT або INSTALLED через UART. Рух при перевезенні дає MOVE без тривоги; рух після встановлення дає DISTURBANCE. Удар і стійка зміна світла встановлюють alarm в обох режимах. Світло означає можливий доступ, а не доведене відкриття кришки.

## Збірка та перевірки
```sh
make clean
make build
make test
make coverage
python3 tools/analyze_capture.py
```

Docker-образ cargoseal-build:lab2 містить ARM GCC 10.3.1. make build створює firmware/build/firmware.elf, firmware.hex і firmware.bin; файли виключені з Git. make test виконує шість груп C tests та два Python regression tests. Перевірка include забороняє HAL і Board у переносимих Sensors/Policy. Coverage переносимих модулів: 100% рядків, 94.44..100% напрямків гілок. Board runtime перевірено з fake HAL; сам апаратний I2C1 adapter перевірено реальними сигналами Wokwi.

## Запуск та команди
У VS Code відкрити саме цю папку, після make build виконати Wokwi: Start Simulator. wokwi.toml задає власні HEX/ELF. Використати доступну ліцензію; trial для цієї роботи не активували.

Без доступу до VS Code ліцензії можна використати тимчасовий онлайн ST Nucleo HAL loader: замінити diagram.json власним, у палітрі редактора вибрати Upload Firmware and Start Simulation… і завантажити саме firmware/build/firmware.hex. Не натискати звичайний Start для компіляції main.c чужого прикладу. Саме так виконано підтверджені записи, без Save/Fork.

До запуску задати IMU ax=ay=0,az=1, gyro=0 та LDR 0.1 lux. Повторний старт браузерної сесії може зберігати попередні значення ползунків, їх потрібно повернути вручну. За 20 відліками LDR формується E0; у підтвердженому run raw=4063,E0=32, ready на time_ms=961.

USART2 115200 8N1. Кожну команду завершувати LF:
```text
STATUS
SELFTEST
READ IMU
MODE TRANSPORT
MODE INSTALLED
ACK
IMU ADDRESS 0x69
IMU ADDRESS 0x68
```
ACK дає ERR ACTIVE, поки IMU invalid або сигнали не повернулися в норму на 500 ms. B1 не має прикладної функції. У тривозі LED PA5 постійно HIGH; без тривоги перемикається раз на секунду, повний цикл 2 s.

## Перевірені результати
13 жовтня 2026 отримано власні записи Wokwi, незалежно декодовано sigrok і відкрито I²C у PulseView. У фінальному VCD 36 подій, fault/recovery, три одночасні входи та переходи при HIGH PIR. SCL 100 kHz, медіанний період IMU 19.99785 ms, ADC 49.99745 ms. Повні MODE/STATUS/ERR завершуються через 1.497..36.643 ms після LF. 20 STATUS/s перевірено timed fake UART, не заявляється як UI-навантаження Wokwi.

Прототип не має фізичної пломби, енергонезалежного журналу або мережевих сповіщень. Фізичні струми, шум, дальність PIR і реальні удари не вимірювали. Нахил у спокої не є окремою подією; норма прискорення не визначає рівномірну швидкість.

## Документація
- [PRD і статус T01..T12](docs/PRD.md)
- [Pinout та монтаж](docs/pinout.md)
- [Компоненти й межі моделей](docs/hardware-components.md)
- [Архітектура](docs/architecture.md)
- [UART](docs/uart-protocol.md)
- [Тести й coverage](docs/testing.md)
- [Власні записи, декодування та вимірювання](docs/captures/analysis.md)
- [Журнал проблем](docs/problems.md)
- [Звіт](docs/lab2-report.md)

Історія містить 10 змістовних етапів ЛР2. За окремим указанням користувача author/committer metadata розподілено по 25 вересня–12 жовтня 2026; фактичні дати журналів, записів та CI збережено.

Основа: [STM32 HAL example for Wokwi](https://github.com/wokwi/stm32-hello-wokwi). Ліцензії прикладу, ST та CMSIS збережено.
