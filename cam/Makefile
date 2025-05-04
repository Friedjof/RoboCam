PLATFORMIO = ~/.platformio/penv/bin/platformio
BOARD = esp32cam
BAND = 115200

all: build

build:
	$(PLATFORMIO) run --environment $(BOARD)

upload: build
	$(PLATFORMIO) run --target upload --environment $(BOARD)

monitor:
	$(PLATFORMIO) device monitor --environment $(BOARD) --baud $(BAND)

clean:
	$(PLATFORMIO) run --target clean --environment $(BOARD)

flash: upload monitor

start: clean flash monitor

.PHONY: all build upload monitor clean flash start
