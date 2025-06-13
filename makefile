build:
	g++ -Iinclude -o pmp main.cpp commands/init.cpp commands/run.cpp commands/install.cpp commands/uninstall.cpp commands/utils.cpp

install:
	sudo cp pmp /usr/local/bin/
	# copy templates folder
	sudo mkdir -p /usr/local/share/pmp/
	sudo cp -r templates /usr/local/share/pmp/
	sudo chmod -R 755 /usr/local/share/pmp/templates

build-install: build install

build-windows:
	@echo Building for Windows...
	@echo Ensure you have MinGW installed and in your PATH.
	g++ -Iinclude -o pmp main.cpp commands/init.cpp commands/run.cpp commands/install.cpp commands/uninstall.cpp commands/utils.cpp

install-windows:
	@echo Installing for Windows...
	-cmd /c "rmdir /s /q C:\tools\pmp || exit 0"
	cmd /c "mkdir C:\tools\pmp"
	cmd /c "copy /Y pmp.exe C:\tools\pmp\\"
	cmd /c "xcopy /E /I templates C:\tools\pmp\\templates"
	@echo Installation complete. Files copied to C:\tools\pmp

build-install-windows: build-windows install-windows