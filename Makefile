IMAGE ?= cargoseal-build:lab2

.PHONY: image build clean test coverage check flash monitor

image:
	docker build -t $(IMAGE) docker

build: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware all

clean:
	$(MAKE) -C firmware clean
check:
	python3 tools/check_layers.py
	python3 tools/check_wiring.py
test: check
	$(MAKE) -C tests test
coverage: image check
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace --user "$(shell id -u):$(shell id -g)" $(IMAGE) python3 tools/coverage.py
flash:
	@printf 'Form B: use Wokwi: Start Simulator after make build.\n'
monitor:
	@printf 'Form B: use Wokwi Terminal, 115200 8N1, commands terminated by LF.\n'
