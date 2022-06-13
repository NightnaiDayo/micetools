BUILD_DIR := build
BUILD_DIR_32 := $(BUILD_DIR)/build32
BUILD_DIR_64 := $(BUILD_DIR)/build64
DIST_DIR := dist

BUILD_DRIVE = M:

MICE_32 = "$(BUILD_DIR_32)/src\mice.exe"
MICE_64 = "$(BUILD_DIR_64)/src\mice.exe"

VCVARS_32 = C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat
VCVARS_64 = C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat

.ONESHELL:

.PHONY: all
all: mice86 dist

mice86:
	@subst $(BUILD_DRIVE) .
	@cd /D $(BUILD_DRIVE) \
	& "$(VCVARS_32)" \
	& meson setup --cross cross-32.ini $(BUILD_DRIVE)\$(BUILD_DIR_32) \
	& meson compile -C $(BUILD_DRIVE)\$(BUILD_DIR_32)
	@subst $(BUILD_DRIVE) /D

mice64:
	@subst $(BUILD_DRIVE) .
	@cd $(BUILD_DRIVE) \
	& "$(VCVARS_64)" \
	& meson setup --cross cross-64.ini $(BUILD_DRIVE)\$(BUILD_DIR_64) \
	& meson compile -C $(BUILD_DRIVE)\$(BUILD_DIR_64)
	@subst $(BUILD_DRIVE) /D

.PHONY: clean
clean:
	@del /S /F /Q $(BUILD_DIR)
	@rmdir /S /Q $(BUILD_DIR)
	@del /S /F /Q $(DIST_DIR)
	@rmdir /S /Q $(DIST_DIR)

.PHONY: dist
dist:
	@-mkdir $(DIST_DIR)
	@copy /Y "$(BUILD_DIR_32)/src/micetools/micekeychip\micekeychip.exe" "$(DIST_DIR)/micekeychip.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/micepatch\micepatch.exe" "$(DIST_DIR)/micepatch.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/lib/libpcp\libpcp.lib" "$(DIST_DIR)/libpcp.lib"

	@copy /Y "$(BUILD_DIR_32)/src/micetools/launcher\mice.exe" "$(DIST_DIR)/mice86.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/dll\mice.dll" "$(DIST_DIR)/mice86.dll"
#	@copy /Y "$(BUILD_DIR_64)/src/micetools/launcher\mice.exe" "$(DIST_DIR)/mice64.exe"
#	@copy /Y "$(BUILD_DIR_64)/src/micetools/dll\mice.dll" "$(DIST_DIR)/mice64.dll"

	@xcopy /E /H /C /R /Q /Y src\system "$(DIST_DIR)\system/*"
	@xcopy /E /H /C /R /Q /Y src\tools "$(DIST_DIR)\tools/*"
