#!/usr/bin/python

import sys

tmpl_fname = sys.argv[1]
out_fname = sys.argv[2]
nstores = int(sys.argv[3])
nnoops = int(sys.argv[4])
nnoops_clean = int(sys.argv[5])

def write_asm_body(out) :
	array_size = nstores
	if nstores < 16:
		array_size = 16
	out.write("        uint64_t movq[4];\n")
	out.write("        __m256i avx_in, avx_out[" + str(2 * array_size) + "];\n")
	out.write("        // Make sure that we are moving with 64B steps to write to a new cache line each time\n")
	out.write("        asm volatile (\n");
	i = 0
	while (i < nstores):
		line = "            "
		line += "\"vmovntdq %%ymm0, " + str(i * 64) + "(%[array])\\n\"\n"
		out.write(line)
		i += 1

	out.write("            : // No output\n")
	out.write("            : [array] \"r\" (avx_out), [in] \"r\" (&avx_in), [mvqptr] \"r\" (movq)\n");
	out.write("            : \"memory\", \"%xmm0\");\n")

with open(tmpl_fname, 'r') as f:
	tmpl = f.readlines()

out = open(out_fname, 'w');

for l in tmpl:
	if "asm_code" in l:
		write_asm_body(out)
	elif "count_init" in l:
		out.write("    int store_cnt = " + str(nstores) + ";\n")
	else:
		out.write(l)
