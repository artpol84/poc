
COMMON = basic_ib.c
RC = basic_rc.c $(COMMON) -libverbs

all:
	gcc -o consume_ib_obj -g -O0 consume_ib_obj.c $(RC)

clean:
	rm -f basic_ud_send basic_ud_recv basic_rc_send basic_rc_recv
