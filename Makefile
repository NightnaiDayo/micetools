BUILD_DIR := build
BUILD_DIR_32 := $(BUILD_DIR)\build32
BUILD_DIR_64 := $(BUILD_DIR)\build64
COMMIT := $(shell git rev-parse --short HEAD)
DIST_DIR := dist

BUILD_DRIVE := M:

MICE := "$(BUILD_DIR)/src\mice.exe"

# VCVARS := "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat"

# For windows XP:
VCVARS_32 := "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat" -vcvars_ver=14.16
VCVARS_64 := "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" -vcvars_ver=14.16

.ONESHELL:

.PHONY: all
all: mice dist
# all: mice mice64 dist

.PHONY: mice
mice:
	-@subst $(BUILD_DRIVE) .
	@cd /D $(BUILD_DRIVE) \
	& $(VCVARS_32) \
	& meson.py setup --cross cross-32.ini $(BUILD_DRIVE)\$(BUILD_DIR_32) \
	& meson.py configure -Dmice_version=$(COMMIT) $(BUILD_DRIVE)\$(BUILD_DIR_32) \
	& meson.py compile -C $(BUILD_DRIVE)\$(BUILD_DIR_32)
	@subst $(BUILD_DRIVE) /D

.PHONY: mice64
mice64:
	-@subst $(BUILD_DRIVE) .
	@cd /D $(BUILD_DRIVE) \
	& $(VCVARS_64) \
	& meson.py setup --cross cross-64.ini $(BUILD_DRIVE)\$(BUILD_DIR_64) \
	& meson.py configure -Dmice_version=$(COMMIT) $(BUILD_DRIVE)\$(BUILD_DIR_64) -Dwin64=true \
	& meson.py compile -C $(BUILD_DRIVE)\$(BUILD_DIR_64)
	@subst $(BUILD_DRIVE) /D

.PHONY: clean
clean:
	@del /S /F /Q $(BUILD_DIR)
	@rmdir /S /Q $(BUILD_DIR)
	@del /S /F /Q $(DIST_DIR)
	@rmdir /S /Q $(DIST_DIR)

.PHONY: dist
dist:
	@-mkdir $(DIST_DIR) > NUL 2>&1
	@-mkdir $(DIST_DIR)\util > NUL 2>&1
	@-mkdir $(DIST_DIR)\system_dummy > NUL 2>&1
	@-mkdir $(DIST_DIR)\Execute > NUL 2>&1
	@-mkdir $(DIST_DIR)\Execute\Z > NUL 2>&1
	@-mkdir $(DIST_DIR)\Execute\S > NUL 2>&1

	@copy /Y "$(BUILD_DIR_32)/src/micetools/micekeychip\micekeychip.exe" "$(DIST_DIR)/micekeychip.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/micemaster\micemaster.exe" "$(DIST_DIR)/micemaster.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/lib/libpcp\libpcp.lib" "$(DIST_DIR)/libpcp.lib"

	@copy /Y "$(BUILD_DIR_32)/src/micetools/launcher\mice.exe" "$(DIST_DIR)/mice.exe"
#	@copy /Y "$(BUILD_DIR_32)/src/micetools/launcher\mice.pdb" "$(DIST_DIR)/mice.pdb"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/dll\mice.pdb" "$(DIST_DIR)/mice.pdb"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/dll\mice.dll" "$(DIST_DIR)/mice.dll"

#	@copy /Y "$(BUILD_DIR_64)/src/micetools/launcher\mice64.exe" "$(DIST_DIR)/mice64.exe"
#   @copy /Y "$(BUILD_DIR_64)/src/micetools/launcher\mice64.pdb" "$(DIST_DIR)/mice64.pdb"
#	@copy /Y "$(BUILD_DIR_64)/src/micetools/dll\mice64.pdb" "$(DIST_DIR)/mice64.pdb"
#	@copy /Y "$(BUILD_DIR_64)/src/micetools/dll\mice64.dll" "$(DIST_DIR)/mice64.dll"

	@copy /Y "$(BUILD_DIR_32)/src/micetools/miceboot\miceprestartup.exe" "$(DIST_DIR)/Execute/miceprestartup.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/miceboot\micestartup.exe" "$(DIST_DIR)/Execute/micestartup.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/miceboot\mxmaster.exe" "$(DIST_DIR)/Execute/S/mxmaster.exe"

	@copy /Y "$(BUILD_DIR_32)/src/micetools/util\*.exe" "$(DIST_DIR)/util/"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/util\*.pdb" "$(DIST_DIR)/util/"

	@xcopy /E /H /C /R /Q /Y src\system "$(DIST_DIR)\system/*"
	@xcopy /E /H /C /R /Q /Y src\patches "$(DIST_DIR)\patches/*"
