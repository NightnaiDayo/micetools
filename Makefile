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
	& meson.py setup --cross cross-32.ini $(BUILD_DRIVE)\$(BUILD_DIR) \
	& meson.py compile -C $(BUILD_DRIVE)\$(BUILD_DIR)
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
	@copy /Y "$(BUILD_DIR)/src/micetools/miceboot\mxmaster.exe" "$(DIST_DIR)/Execute/S/mxmaster.exe"

	@copy /Y "$(BUILD_DIR)/src/micetools/util\*.exe" "$(DIST_DIR)/util/"
	@copy /Y "$(BUILD_DIR)/src/micetools/util\*.pdb" "$(DIST_DIR)/util/"

	@xcopy /E /H /C /R /Q /Y src\system "$(DIST_DIR)\system/*"
	@xcopy /E /H /C /R /Q /Y src\patches "$(DIST_DIR)\patches/*"
