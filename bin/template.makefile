default: init build
build: init ${LIBOUT}
rebuild: clean init build

${LIBOUT}:   ${OBJS}
	${AR} crs ${LIBOUT} $(OBJS)
	mv ${LIBOUT} ${OUTPUTS}/lib/

.c.o: ${HDRS}
	${CC} ${CFLAGS} -c $*.c

.cpp.o: ${HDRS}
	${CPP} ${CPPFLAGS} -c $*.cpp
	
init:
	cp ${HDRS} ${OUTPUTS}/include/
	
compile: 
	${CPP} ${CFLAGS} ${CODENAME}.cpp -o ./${OUTPUTS}/codes/${CODENAME}.elf -L${OUTPUTS}/lib/ -I./${OUTPUTS}/include ${LIBSTRING} -lm -lm
	avr-objcopy -O ihex -R .eeprom ./${OUTPUTS}/codes/${CODENAME}.elf ./${OUTPUTS}/codes/${CODENAME}.hex

upload:
	${CPP} ${CFLAGS} ${CODENAME}.cpp -o ./${OUTPUTS}/codes/${CODENAME}.elf -L${OUTPUTS}/lib/ -I./${OUTPUTS}/include ${LIBSTRING} -lm -lm
	avr-objcopy -O ihex -R .eeprom ./${OUTPUTS}/codes/${CODENAME}.elf ./${OUTPUTS}/codes/${CODENAME}.hex			
	avrdude -c ${PROGRAMMER} -P ${COMPORT} -p ${PARTNO} -U flash:w:./${OUTPUTS}/codes/${CODENAME}.hex
	
seteeprom:
	avrdude -c ${PROGRAMMER} -P ${COMPORT} -p ${PARTNO} -U lfuse:w:0xff:m -U hfuse:w:0xd6:m -U efuse:w:0xfd:m

defaultfuse:
	avrdude -c ${PROGRAMMER} -P ${COMPORT} -p ${PARTNO} -U lfuse:w:0xff:m -U hfuse:w:0xDE:m -U efuse:w:0xFD:m
	
clean:
	rm -f $(OBJS) 

clean2:
	rm -f ./${OUTPUTS}/codes/${CODENAME}.elf 
	rm -f ./${OUTPUTS}/codes/${CODENAME}.hex
