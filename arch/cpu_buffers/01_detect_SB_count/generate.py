#!/usr/bin/python

import sys, os, inspect

currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0,parentdir) 

import litmus_test_lib as lib

class sb_count(lib.litmus_test) :
	def test_body(self) :
		self.out.write("        uint64_t array[" + str(2 * self.nstores) + "];\n");
		self.out.write("        asm volatile (\n");
		i = 0
		while (i < self.nstores):
			line = "            "
			line += "\"movq $" + str(i) + ", "
			line += str(i * 16) + "(%[array])\\n\"\n"
			self.out.write(line)
			i += 1
		i = 0
		while (i < self.nnoops):
			line = "            "
			line += "\"nop\\n\"\n"
			self.out.write(line)
			i += 1
		self.out.write("            : // No output\n")
		self.out.write("            : [array] \"r\" (array)\n")
		self.out.write("            : \"memory\");\n")

	def reset_cpu(self) :
		self.out.write("        // No CPU store resets in this test\n")


x = sb_count()
x.create_test()