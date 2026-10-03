CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g
BUILD = build/$(shell uname -s)-$(shell uname -m)
.PHONY: test clean
test: $(BUILD)/sensors $(BUILD)/light $(BUILD)/policy
	./$(BUILD)/sensors
	./$(BUILD)/light
	./$(BUILD)/policy
$(BUILD)/policy: test_policy.c ../firmware/Policy/container.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@
$(BUILD)/light: test_light.c ../firmware/Sensors/light.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@
$(BUILD)/sensors: test_sensors.c ../firmware/Sensors/mpu6050.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@
clean:
	rm -rf build

