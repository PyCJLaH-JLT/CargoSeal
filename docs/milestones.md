# Етапи CargoSeal
ЛР1 збережена у main після merge PR №1. У власній ЛР2 реалізовано pinout і .ioc, переносимі контракти, MPU6050 driver, LDR/PIR, policy, LF-команди, Board integration, host/coverage/CI, перевірку реальних сигналів і документацію проєкту. Кожен етап має окремий змістовний коміт, разом 10.

Фінальна перевірка 13 жовтня 2026: Docker ARM build, шість C test groups, два regression tests VCD, sanitizer, coverage, власні Wokwi recordings з 36 подіями, ID/burst/NACK. Ползунки Wokwi обмежують точні крайні значення; такі значення, timer boundaries і wrap перевіряються на хості.

Наступна технічна робота, якщо з'явиться фізична плата: перевірити напругу PIR OUT, сумарні pull-up, струми breakout, rise/fall I²C, source impedance ADC та stack watermark. Це межі форми B, не невиконані функції PRD.
