#!/usr/bin/python

import sys, os, inspect

currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0,parentdir) 

import litmus_test_lib as lib

class wcb_sb_interop(lib.litmus_test) :
	def test_body(self) :
		array_size = self.nstores
		if self.nstores < 16:
			array_size = 16
		self.out.write("        uint64_t movq[4];\n")
		self.out.write("        __m256i avx_in, avx_out[" + str(2 * array_size) + "];\n")
		self.out.write("        // Make sure that we are moving with 64B steps to write to a new cache line each time\n")
		self.out.write("        asm volatile (\n");
		i = 0
		while (i < self.nstores):
			line = "            "
			line += "\"vmovntdq %%ymm0, " + str(i * 64) + "(%[array])\\n\"\n"
			self.out.write(line)
			i += 1

		i = 0
		while (i < self.nnoops) :
			line = "            "
			line += "\"nop\\n\"\n"
			self.out.write(line)
			i += 1
    
		self.out.write("            : // No output\n")
		self.out.write("            : [array] \"r\" (avx_out), [in] \"r\" (&avx_in), [mvqptr] \"r\" (movq)\n");
		self.out.write("            : \"memory\", \"%xmm0\");\n")

x = wcb_sb_interop()
x.create_test()
