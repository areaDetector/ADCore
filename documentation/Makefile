HTMLFLAGS = --book --duplex --no-numbered --format pdf14 --title --size letter

%.pdf: %.html
	-htmldoc $(HTMLFLAGS) --outfile $@ $<

all: areaDetectorDoc.pdf doxygen iocBootDocs install

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
