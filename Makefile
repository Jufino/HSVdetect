C=$(CROSS_COMPILE)gcc -Wall -g
CPP=$(CROSS_COMPILE)g++ -Wall -g

GFLAGS=`pkg-config --libs --cflags gtk+-2.0 gmodule-2.0`
OFLAGS=`pkg-config --libs --cflags opencv`

TARGET=HSVdetect

HSVdetect: HSVdetect.cpp
	$(CPP) HSVdetect.cpp -o HSVdetect $(OFLAGS)

all: $(TARGET)
	@echo "All done"



