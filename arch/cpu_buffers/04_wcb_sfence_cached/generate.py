#!/usr/bin/python

import sys, os, inspect

currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0,parentdir) 

import litmus_test_lib as lib

full_writes = 1

class wcb_w_sfence(lib.litmus_test) :
	def test_body(self) :
		self.out.write("        uint64_t movq[" + str(self.nstores) + "];\n")
		self.out.write("        __m256i avx_in, avx_out_base[3], *avx_out = avx_out_base;\n")
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
		self.out.write("            \"sfence\\n\"\n")

	    # Place requested number of regular stores
		i = 0
		while (i < self.nstores):
			line = "            "
			line += "\"movq $" + str(i) + ", "
			line += str(i * 8) + "(%[mvqptr])\\n\"\n"
			self.out.write(line)
			i += 1
	
	    # Execute requested number of no-ops as an indicator of additional latencies
		i = 0
		while (i < self.nnoops) :
			line = "            "
			line += "\"nop\\n\"\n"
			self.out.write(line)
			i += 1
    
		self.out.write("            : // No output\n")
		self.out.write("            : [array] \"r\" (avx_out), [in] \"r\" (&avx_in), [mvqptr] \"r\" (movq)\n");
		self.out.write("            : \"memory\", \"%xmm0\");\n")


if ( len(sys.argv) > 6 ) :
	full_writes = 0

x = wcb_w_sfence()
x.create_test()
