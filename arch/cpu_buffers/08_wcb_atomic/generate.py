#!/usr/bin/python

import sys, os, inspect

currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0,parentdir) 

import litmus_test_lib as lib

full_writes = 1

class wcb_sfence_atomic(lib.litmus_test) :
	def test_body(self) :
		self.out.write("        __m256i avx_in, avx_out_base[3], *avx_out = avx_out_base;\n")
		self.out.write("        int lock = 0;\n")
		self.out.write("        if( ((uint64_t)avx_out) % 64 ) { avx_out++; }\n")
		self.out.write("        // Make sure that we are moving with 64B steps to write to a new cache line each time\n")
		self.out.write("        asm volatile (\n");

		# Put a long latency Non-Temporal (NT) operation
		line = "            "
		line += "\"vmovntdq %%ymm0, 0(%[array])\\n\"\n"
		self.out.write(line)

		if (full_writes ) :
			line = "            "
			line += "\"vmovntdq %%ymm0, 32(%[array])\\n\"\n"
			self.out.write(line)

		# Put an SFENCE
    		# self.out.write("            \"sfence\\n\"\n")

		line = "            "
		line += "\"lock; decl (%[lock])\\n\"\n"
		self.out.write(line)
    
		self.out.write("            : // No output\n")
		self.out.write("            : [array] \"r\" (avx_out), [in] \"r\" (&avx_in), [lock] \"r\" (&lock)\n");
		self.out.write("            : \"memory\", \"%xmm0\");\n")

if ( len(sys.argv) > 6 ) :
	full_writes = 0

x = wcb_sfence_atomic()
x.create_test()
