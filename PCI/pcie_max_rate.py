# Script to compute max rate for PCIe writes for NVIDIA CX devices (with 64/128B CQEs)
from math import ceil

MAX_TLP   = 256
MAX_S2CQE1= 32
MAX_S2CQE2= 64

CQE1_SIZE    = 64
CQE2_SIZE    = 128

# PCIe Gen 3 and Gen 4
START_SIZE  = 1  # PHY Layer
END_SIZE    = 1  # PHY Layer
SEQ_N_SIZE  = 2  # Data Link Layer
LCRC_SIZE   = 4  # Data Link Layer
WR_HDR_SIZE = 12 # Transaction Layer
EDRC_SIZE   = 4  # Transaction Layer

# Data is transmitted in form of a stream, and in full load there is no spacing among TLPs
TLP_HDR_SIZE = START_SIZE + END_SIZE + SEQ_N_SIZE + LCRC_SIZE + WR_HDR_SIZE + EDRC_SIZE

PCI_LANES   = 16
LANE_RATE_3 = 8 # Gbps, PCIe Gen 3
LANE_RATE_4 = 16 # Gbps, PCIe Gen 4
ENCODING    = 128.0/130 # PCIe Gen 3 and Gen 4

max_pcie3_bw = PCI_LANES * LANE_RATE_3 * ENCODING
max_pcie4_bw = PCI_LANES * LANE_RATE_4 * ENCODING


# input: msg_size in bytes, output: actual transmitted bytes considering CQE and TLP
def actual_bytes(msg_size):
    if msg_size <= MAX_S2CQE1:
        return CQE1_SIZE+TLP_HDR_SIZE
    elif msg_size <= MAX_S2CQE2:
        return CQE2_SIZE+TLP_HDR_SIZE
    else:
        return CQE1_SIZE+TLP_HDR_SIZE+msg_size+TLP_HDR_SIZE*ceil(msg_size/MAX_TLP)


# input: msg_size in bytes, output: max rate in MMPS
def max_rate(msg_size, pcie_ver=3):
    if (pcie_ver==3):
        return max_pcie3_bw * 1000.0 / (8*actual_bytes(msg_size))
    elif (pcie_ver==4):
        return max_pcie4_bw * 1000.0 / (8*actual_bytes(msg_size))


# input: msg_size in bytes, output max effective rate just for the msg, in Gbps
def max_bw(msg_size, pcie_ver=3):
    return max_rate(msg_size, pcie_ver)*msg_size*8/1000


SW_HDR_SIZE=16
payloads = [SW_HDR_SIZE+(2**x) for x in list(range(0,15))]
print ("Msg(B):\tactual(B), \tmax rate, \tmax bw")
for x in payloads:
    print (str(x-SW_HDR_SIZE)+": \t"+str(actual_bytes(x))+", \t"+str(max_rate(x,3))+", \t"+str(max_bw(x,3)))
