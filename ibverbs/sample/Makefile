
COMMON = basic_ib.c basic_tcp.c
UD = basic_ud.c $(COMMON) -libverbs
RC = basic_rc.c $(COMMON) -libverbs

all:
	gcc -o basic_ud_send -g -O0 basic_ud_send.c $(UD)
	gcc -o basic_ud_recv -g -O0 basic_ud_recv.c $(UD)
	gcc -o basic_rc_send -g -O0 basic_rc_send.c $(RC)
	gcc -o basic_rc_recv -g -O0 basic_rc_recv.c $(RC)

clean:
	rm -f basic_ud_send basic_ud_recv basic_rc_send basic_rc_recv
