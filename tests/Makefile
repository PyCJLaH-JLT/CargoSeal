CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g
BUILD = build/$(shell uname -s)-$(shell uname -m)
.PHONY: test clean
test: $(BUILD)/sensors $(BUILD)/light $(BUILD)/policy $(BUILD)/commands $(BUILD)/acceptance $(BUILD)/runtime $(BUILD)/custody $(BUILD)/diagnostics
	./$(BUILD)/sensors
	./$(BUILD)/light
	./$(BUILD)/policy
	./$(BUILD)/commands
	./$(BUILD)/acceptance
	./$(BUILD)/runtime
	./$(BUILD)/custody
	./$(BUILD)/diagnostics
$(BUILD)/custody: test_custody.c ../firmware/Policy/custody_log.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@
$(BUILD)/diagnostics: test_diagnostics.c ../firmware/Board/diagnostics.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -DCS_HOST_TEST $^ -o $@
$(BUILD)/runtime: test_runtime.c fakes/imu_bus.c fakes/lab3_board.c ../firmware/Board/runtime.c ../firmware/Board/diagnostics.c ../firmware/Policy/custody_log.c ../firmware/Policy/container.c ../firmware/Policy/commands.c ../firmware/Sensors/mpu6050.c ../firmware/Sensors/light.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -DCS_HOST_TEST -Ifakes $^ -lm -o $@
$(BUILD)/acceptance: test_acceptance.c ../firmware/Policy/container.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@
$(BUILD)/commands: test_commands.c ../firmware/Policy/container.c ../firmware/Policy/commands.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@
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

