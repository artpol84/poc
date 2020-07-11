#!/usr/bin/python
# Copyright (c) 2020      Mellanox Technologies, Inc.
#                         All rights reserved.

import re
import os
import sys
import texttable as tt

def process(fname):
    with open(fname) as f:
        text = f.readlines()
    f.close()

    regex1="\|\|(.+)\|\|(.+)\|\|(\||=).*\|\|"
    regexc1 = re.compile(regex1)

    regex2="\|\|(.+)\|\|(.+)\|\|"
    regexc2 = re.compile(regex2)

    summary = {}
    order = {}
    seqnum = 0
    for l in text:
        m = regexc1.match(l)
        name = None
        val = None
        if( None != m ):
            name = m.group(1).strip()
            val =  m.group(2).strip()
        else:
            m = regexc2.match(l)
            if( None != m ):
                name = m.group(1).strip()
                val =  m.group(2).strip()
        if( None != name):
            val = val.replace(',','')
            if( not val.isdigit() ):
                continue
            if not name in summary.keys():
                summary[name] = {}
                summary[name]["count"] = 1
                summary[name]["val"] = int(val)
                summary[name]["max"] = int(val)
                order[seqnum] = name
                seqnum += 1
            else:
                summary[name]["count"] += 1
                summary[name]["val"] += int(val)
                if( int(val) > summary[name]["max"]):
                    summary[name]["max"] = int(val)

    return order, summary

if( len(sys.argv) < 3 ) :
    print "Need file name"
    sys.exit()


order1, summary1 = process(sys.argv[1])
order2, summary2 = process(sys.argv[2])


table = tt.Texttable(200)
table.set_deco(tt.Texttable.HEADER)
table.set_cols_dtype(['t',  # text
                      'i',  # integer
                      'i',  # 
                      'f',  #
                      'i',  # float
                      'i',  # 
                      'i',  # 
                      'f',  #
                      'i',  # float
                   ])
table.set_cols_align(["l", "c", "c", "c", "c", "c", "c", "c", "c"])
table.add_row([ "Counter name", "Samples1", "Max1", "Avg1", "Total1", "Samples2", "Max2", "Avg2", "Total2" ])

for i in xrange(0,len(order1.keys())):
    name = order1[i]
#    print name + " : " + "{0:,d}".format(summary[name])
    smpl1 = summary1[name]["count"]
    smpl2 = summary2[name]["count"]

    max1 = summary1[name]["max"]
    max2 = summary2[name]["max"]

    tot1 = summary1[name]["val"]
    tot2 = summary2[name]["val"]

    avg1 = tot1/smpl1
    avg2 = tot2/smpl2


    table.add_row([ name, smpl1, "{0:,d}".format(max1), "{0:,.0f}".format(avg1), "{0:,d}".format(tot1), 
                          smpl2, "{0:,d}".format(max2), "{0:,.0f}".format(avg2), "{0:,d}".format(tot2)] )
print table.draw()