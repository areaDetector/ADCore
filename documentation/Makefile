HTMLFLAGS = --book --duplex --no-numbered --format pdf14 --title --size letter

%.pdf: %.html
	-htmldoc $(HTMLFLAGS) --outfile $@ $<

all: areaDetectorDoc.pdf doxygen iocBootDocs install

doxygen:
	doxygen

iocBootDocs:
	./makeIOCBootDocs

install:
	cp -f *.html /home/epics/web_software/epics
	cp -f *.png /home/epics/web_software/epics
	cp -f *.jpg /home/epics/web_software/epics
	cp -rfp areaDetectorDoxygenHTML  /home/epics/web_software/epics/

clean:
	rm -rf areaDetectorDoxygenHTML/*


realclean: clean
