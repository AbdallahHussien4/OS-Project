build:
	gcc producer.c -o producer.out 
	gcc consumer.c -o consumer.out

clean:
	rm -f *.out *.txt

all: clean build kill

run:
	./producer.out

kill:
	ipcrm -a
	killall -e producer.out
	killall -e consumer.out 
	killall -e make
