all: doxygen iocBootDocs install

doxygen:
	doxygen

iocBootDocs:
	./makeIOCBootDocs

install:
	cp *.html /home/epics/web_software/epics
	cp *.png /home/epics/web_software/epics
	cp *.jpg /home/epics/web_software/epics
	cp -rp areaDetectorDoxygenHTML  /home/epics/web_software/epics/

clean:
	rm -rf areaDetectorDoxygenHTML/*


realclean: clean
