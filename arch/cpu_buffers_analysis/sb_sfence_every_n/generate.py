#!/usr/bin/python

import sys

tmpl_fname = sys.argv[1]
out_fname = sys.argv[2]
nstores = int(sys.argv[3])
nnoops = int(sys.argv[4])
nnoops_clean = int(sys.argv[5])
every_n = int(sys.argv[6])

def write_mov(out, offs) :
	line = "            "
	line += "\"movq $" + str(offs) + ", "
	line += str(offs * 8) + "(%[array])\\n\"\n"
	out.write(line)

def write_asm_body(out) :
	out.write("        uint64_t array[" + str(nstores) + "];\n");
	out.write("        asm volatile (\n");

	i = 0
	while (i < nstores):
		write_mov(out, i)
		i += 1
		if (i % every_n == 0) :
			out.write("            \"sfence\\n\"\n")

	i = 0
	while (i < nnoops):
		line = "            "
		line += "\"nop\\n\"\n"
		out.write(line)
		i += 1
	out.write("            : // No output\n")
	out.write("            : [array] \"r\" (array)\n")
	out.write("            : \"memory\");\n")

def write_noops(out) :
	out.write("        asm volatile (\n");

	i = 0
	while (i < nnoops_clean) :
		line = "            "
		line += "\"nop\\n\"\n"
		out.write(line)
		i += 1
    
	out.write("            : // No output\n")
	out.write("            : \n");
	out.write("            : \"memory\", \"%xmm0\");\n")


with open(tmpl_fname, 'r') as f:
	tmpl = f.readlines()

out = open(out_fname, 'w');

for l in tmpl:
	if "asm_code" in l:
		write_asm_body(out)
	elif "noop_code" in l:
		write_noops(out)
	elif "count_init" in l:
		out.write("    int store_cnt = " + str(nstores) + ";\n")
	else:
		out.write(l)
