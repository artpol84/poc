
all:
	gcc -o demo_create_mkey -g -O0 demo_create_mkey.c -libverbs
	gcc -o demo_create_mkey_iova -DUSE_IOVA=1 -g -O0 demo_create_mkey.c -libverbs

clean:
	rm -f demo_create_mkey demo_create_mkey_iova
