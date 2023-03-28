BUILD_DIR := build
DIST_DIR := dist

BUILD_DRIVE := M:

MICE := "$(BUILD_DIR)/src\mice.exe"

# VCVARS := "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat"

# For windows XP:
VCVARS := "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat" -vcvars_ver=14.16

.ONESHELL:

.PHONY: all
all: mice dist

mice:
	-@subst $(BUILD_DRIVE) .
	@cd /D $(BUILD_DRIVE) \
	& $(VCVARS) \
	& meson setup --cross cross-32.ini $(BUILD_DRIVE)\$(BUILD_DIR) \
	& meson compile -C $(BUILD_DRIVE)\$(BUILD_DIR)
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

	@copy /Y "$(BUILD_DIR)/src/micetools/micekeychip\micekeychip.exe" "$(DIST_DIR)/micekeychip.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/micemaster\micemaster.exe" "$(DIST_DIR)/micemaster.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/lib/libpcp\libpcp.lib" "$(DIST_DIR)/libpcp.lib"

	@copy /Y "$(BUILD_DIR)/src/micetools/launcher\mice.exe" "$(DIST_DIR)/mice.exe"
#   @copy /Y "$(BUILD_DIR)/src/micetools/launcher\mice.pdb" "$(DIST_DIR)/mice.pdb"
	@copy /Y "$(BUILD_DIR)/src/micetools/dll\mice.pdb" "$(DIST_DIR)/mice.pdb"
	@copy /Y "$(BUILD_DIR)/src/micetools/dll\mice.dll" "$(DIST_DIR)/mice.dll"

	@copy /Y "$(BUILD_DIR)/src/micetools/miceboot\miceprestartup.exe" "$(DIST_DIR)/Execute/miceprestartup.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/miceboot\micestartup.exe" "$(DIST_DIR)/Execute/micestartup.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/miceboot\TrueCrypt.exe" "$(DIST_DIR)/Execute/TrueCrypt.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/miceboot\mxmaster.exe" "$(DIST_DIR)/Execute/S/mxmaster.exe"

#	@copy /Y "$(BUILD_DIR)/src/micetools/micepatch\micepatch.exe" "$(DIST_DIR)/util/micepatch.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/util\micedump.exe" "$(DIST_DIR)/util/micedump.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/util\micedump.pdb" "$(DIST_DIR)/util/micedump.pdb"
	@copy /Y "$(BUILD_DIR)/src/micetools/util\micetinker.exe" "$(DIST_DIR)/util/micetinker.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/util\micemonitor.exe" "$(DIST_DIR)/util/micemonitor.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/util\exio_test.exe" "$(DIST_DIR)/util/exio_test.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/util\dongleDecrypt.exe" "$(DIST_DIR)/util/dongleDecrypt.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/util\testBin.exe" "$(DIST_DIR)/util/testBin.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/util\micegbdisk.exe" "$(DIST_DIR)/util/micegbdisk.exe"

	@copy /Y "$(BUILD_DIR)/src/micetools/system_dummy\dummymaster\dummymaster.exe" "$(DIST_DIR)/system_dummy/dummymaster.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/system_dummy\dummyinstaller\dummyinstaller.exe" "$(DIST_DIR)/system_dummy/dummyinstaller.exe"
	@copy /Y "$(BUILD_DIR)/src/micetools/system_dummy\dummykeychip\dummykeychip.exe" "$(DIST_DIR)/system_dummy/dummykeychip.exe"

	@copy /Y "src/micetools/miceboot\TrueCrypt.cmd" "$(DIST_DIR)/Execute/TrueCrypt.cmd"

	@xcopy /E /H /C /R /Q /Y src\system "$(DIST_DIR)\system/*"
	@xcopy /E /H /C /R /Q /Y src\tools "$(DIST_DIR)\tools/*"
	@xcopy /E /H /C /R /Q /Y src\patches "$(DIST_DIR)\patches/*"
