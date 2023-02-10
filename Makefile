BUILD_DIR := build
BUILD_DIR_32 := $(BUILD_DIR)/build32
BUILD_DIR_64 := $(BUILD_DIR)/build64
DIST_DIR := dist

BUILD_DRIVE := M:

MICE_32 := "$(BUILD_DIR_32)/src\mice.exe"
MICE_64 := "$(BUILD_DIR_64)/src\mice.exe"

# VCVARS_32 := "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat"
# VCVARS_64 := "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat"

# For windows XP:
VCVARS_32 := "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat" -vcvars_ver=14.16
VCVARS_64 := "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" -vcvars_ver=14.16

.ONESHELL:

.PHONY: all
all: mice86 dist

mice86:
	-@subst $(BUILD_DRIVE) .
	@cd /D $(BUILD_DRIVE) \
	& $(VCVARS_32) \
	& meson setup --cross cross-32.ini $(BUILD_DRIVE)\$(BUILD_DIR_32) \
	& meson compile -C $(BUILD_DRIVE)\$(BUILD_DIR_32)
	@subst $(BUILD_DRIVE) /D

mice64:
	-@subst $(BUILD_DRIVE) .
	@cd $(BUILD_DRIVE) \
	& $(VCVARS_64) \
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
	@-mkdir $(DIST_DIR) > NUL 2>&1
	@-mkdir $(DIST_DIR)\util > NUL 2>&1
	@-mkdir $(DIST_DIR)\Execute > NUL 2>&1
	@-mkdir $(DIST_DIR)\Execute\Z > NUL 2>&1
	@-mkdir $(DIST_DIR)\Execute\S > NUL 2>&1

	@copy /Y "$(BUILD_DIR_32)/src/micetools/micekeychip\micekeychip.exe" "$(DIST_DIR)/micekeychip.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/micemaster\micemaster.exe" "$(DIST_DIR)/micemaster.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/lib/libpcp\libpcp.lib" "$(DIST_DIR)/libpcp.lib"

	@copy /Y "$(BUILD_DIR_32)/src/micetools/launcher\mice.exe" "$(DIST_DIR)/mice86.exe"
#	@copy /Y "$(BUILD_DIR_32)/src/micetools/launcher\mice.pdb" "$(DIST_DIR)/mice86.pdb"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/dll\mice.pdb" "$(DIST_DIR)/mice86.pdb"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/dll\mice.dll" "$(DIST_DIR)/mice86.dll"
#	@copy /Y "$(BUILD_DIR_64)/src/micetools/launcher\mice.exe" "$(DIST_DIR)/mice64.exe"
#	@copy /Y "$(BUILD_DIR_64)/src/micetools/dll\mice.dll" "$(DIST_DIR)/mice64.dll"

	@copy /Y "$(BUILD_DIR_32)/src/micetools/miceboot\miceprestartup.exe" "$(DIST_DIR)/Execute/miceprestartup.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/miceboot\micestartup.exe" "$(DIST_DIR)/Execute/micestartup.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/miceboot\TrueCrypt.exe" "$(DIST_DIR)/Execute/TrueCrypt.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/miceboot\mxmaster.exe" "$(DIST_DIR)/Execute/S/mxmaster.exe"

#	@copy /Y "$(BUILD_DIR_32)/src/micetools/micepatch\micepatch.exe" "$(DIST_DIR)/util/micepatch.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/util\micedump.exe" "$(DIST_DIR)/util/micedump.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/util\micetinker.exe" "$(DIST_DIR)/util/micetinker.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/util\micemonitor.exe" "$(DIST_DIR)/util/micemonitor.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/util\exio_test.exe" "$(DIST_DIR)/util/exio_test.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/util\dongleDecrypt.exe" "$(DIST_DIR)/util/dongleDecrypt.exe"
	@copy /Y "$(BUILD_DIR_32)/src/micetools/util\testBin.exe" "$(DIST_DIR)/util/testBin.exe"

	@copy /Y "src/micetools/miceboot\TrueCrypt.cmd" "$(DIST_DIR)/Execute/TrueCrypt.cmd"

	@xcopy /E /H /C /R /Q /Y src\system "$(DIST_DIR)\system/*"
	@xcopy /E /H /C /R /Q /Y src\tools "$(DIST_DIR)\tools/*"
	@xcopy /E /H /C /R /Q /Y src\patches "$(DIST_DIR)\patches/*"
