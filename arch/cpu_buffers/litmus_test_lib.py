#!/usr/bin/python

import sys

class litmus_test :
	def __init__(self) :
		self.tmpl_fname = sys.argv[1]
		self.out_fname = sys.argv[2]
		self.nstores = int(sys.argv[3])
		self.nnoops = int(sys.argv[4])

		with open(self.tmpl_fname, 'r') as f:
			self.tmpl = f.readlines()
		self.out = open(self.out_fname, 'w');

	def reset_cpu(self) :
		# Write the header
		self.out.write("        uint64_t movq_w[4];\n")
		self.out.write("        asm volatile (\n");

		# Force all store ops to be flushed:
		# 1. First post sfence 
		self.out.write("            \"sfence\\n\"\n")
		
		# 2. Next put a store operation that won't complete untill all previous store ops
		line = "            "
		line += "\"movq $1, 0(%[mvqptr])\\n\"\n"
		self.out.write(line)

		# 3. Call CPuID to serialize execution
		self.out.write("            \"movl $0, %%eax\\n\"\n")
		self.out.write("            \"cpuid\\n\"\n")

		# Write the footer    
		self.out.write("            : // No output\n")
		self.out.write("            : [mvqptr] \"r\" (movq_w)\n");
		self.out.write("            : \"memory\", \"%xmm0\", \"eax\", \"ebx\", \"ecx\", \"edx\");\n")

	def test_body(self) :
		self.out.write("    // A stub for derivative classes\n");

	def create_test(self) :
		for l in self.tmpl:
			if "test_body" in l:
				self.test_body()
			elif "reset_code" in l:
				self.reset_cpu()
			elif "count_init" in l:
				self.out.write("    int store_cnt = " + str(self.nstores) + ";\n")
			else:
				self.out.write(l)
