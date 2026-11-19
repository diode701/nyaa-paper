# One entry point for both halves. IDEs are for editing; this file builds.
.DEFAULT_GOAL := help

.PHONY: help daemon gui run-daemon run-gui test e2e smoke clean

help:
	@echo "daemon      build the Rust daemon (nyaa-paperd)"
	@echo "gui         configure + build the Qt/QML client (needs gui/CMakeLists.txt)"
	@echo "run-daemon  run the daemon using /bin/sleep as a fake engine"
	@echo "run-gui     run the Qt client"
	@echo "test        cargo test + the C++ test binaries"
	@echo "e2e         real daemon on a private bus: apply + playlist, desktop untouched"
	@echo "smoke       end-to-end: fake workshop, daemon, client render"
	@echo "clean       remove build outputs"

daemon:
	cd daemon && cargo build

gui:
	cmake -S gui -B gui/build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	cmake --build gui/build

run-daemon:
	cd daemon && NYAA_ENGINE=/bin/sleep RUST_LOG=debug cargo run

run-gui:
	./gui/build/nyaa-paper-qt

test:
	cd daemon && cargo test
	cmake -S gui -B gui/build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	cmake --build gui/build
	cd gui/build && ctest --output-on-failure

e2e: daemon gui
	./scripts/e2e.sh

smoke:
	./scripts/smoke.sh

clean:
	cd daemon && cargo clean
	rm -rf gui/build
