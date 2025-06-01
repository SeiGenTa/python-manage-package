build:
	g++ -o pmp main.cpp commands/init.cpp commands/run.cpp commands/install.cpp

install:
	sudo cp pmp /usr/local/bin/
	# copy templates folder
	sudo mkdir -p /usr/local/share/pmp/
	sudo cp -r templates /usr/local/share/pmp/
	sudo chmod -R 755 /usr/local/share/pmp/templates