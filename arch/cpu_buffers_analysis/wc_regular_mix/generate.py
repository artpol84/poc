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
	out.write("        uint64_t movq[" + str(nstores) + "];\n");
	out.write("        __m256i avx_in, avx_out[1];\n")
	out.write("        // Make sure that we are moving with 64B steps to write to a new cache line each time\n")
	out.write("        asm volatile (\n");

    # Put a long latency Non-Temporal (NT) operation
	line = "            "
	line += "\"vmovntdq %%ymm0, 0(%[array])\\n\"\n"
	out.write(line)

    # Put an SFENCE
	out.write("            \"sfence\\n\"\n")

    # Place requested number of regular stores
	i = 0
	while (i < nstores):
		line = "            "
		line += "\"movq $" + str(i) + ", "
		line += str(i * 8) + "(%[mvqptr])\\n\"\n"
		out.write(line)
		i += 1

    # Execute requested number of no-ops as an indicator of additional latencies
	i = 0
	while (i < nnoops) :
		line = "            "
		line += "\"nop\\n\"\n"
		out.write(line)
		i += 1
    
	out.write("            : // No output\n")
	out.write("            : [array] \"r\" (avx_out), [in] \"r\" (&avx_in), [mvqptr] \"r\" (movq)\n");
	out.write("            : \"memory\", \"%xmm0\");\n")

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
