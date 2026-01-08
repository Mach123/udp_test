all: udp_trx udp_tx udp_rx

udp_trx: udp_trx.c
	gcc -O2 -o udp_trx udp_trx.c -lrt

udp_tx: udp_tx.c
	gcc -O2 -o udp_tx udp_tx.c -lrt

udp_rx: udp_rx.c
	gcc -O2 -o udp_rx udp_rx.c

clean:
	rm udp_trx udp_tx udp_rx

