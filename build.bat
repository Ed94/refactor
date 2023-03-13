@echo off

if not exist build\nul (
	meson setup build
)

echo:
ninja -C build