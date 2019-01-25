#!/usr/bin/python

import sys, os, inspect

currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0,parentdir) 

import litmus_test_lib as lib

class wcb_no_sfence(lib.litmus_test) :
	def test_body(self) :
		self.out.write("        uint64_t movq[" + str(self.nstores) + "];\n");
		self.out.write("        __m256i avx_in, avx_out[1];\n")
		self.out.write("        // Make sure that we are moving with 64B steps to write to a new cache line each time\n")
		self.out.write("        asm volatile (\n");

	    # Put a long latency Non-Temporal (NT) operation
		line = "            "
		line += "\"vmovntdq %%ymm0, 0(%[array])\\n\"\n"
		self.out.write(line)

	    # Place requested number of regular stores
		i = 0
		while (i < self.nstores):
			line = "            "
			line += "\"movq $" + str(i) + ", "
			addr = i % 32
			line += str(addr * 8) + "(%[mvqptr])\\n\"\n"
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

x = wcb_no_sfence()
x.create_test()