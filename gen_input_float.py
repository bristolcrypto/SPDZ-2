#!/usr/bin/python
import re
import math
from subprocess import call

def printusage():
    print """ 
            Format of float_vals.in:
            - First line contains number of values
            - Second line contains a space separated list of integer or float values.
              
            E.g:
            3
            1.337 1338 3.14159
          """

def convertFloat(v):
    # single precision
    vlen = 24
    plen = 8

    if v < 0:
        s = 1
    else:
        s = 0
    if v == 0:
        v = 0
        p = 0
        z = 1
    else:
        p = int(math.floor(math.log(abs(v), 2))) - vlen + 1
        v = int(round(abs(v) * 2 ** (-p)))
        if v == 2 ** vlen:
            p += 1
            v /= 2
        z = 0
        if abs(p) >= 2 ** plen:
            raise Error('Cannot convert %s to float ' \
                                    'with %d exponent bits' % (v, plen))
    return [v, p, z, s]

fnameFloat = "float_vals.in"
fnameGfp = "gfp_vals.in"
regex = re.compile(r'\d+\.\d+')

print "reading float_vals.in ..."
with open(fnameFloat) as f:
    content = f.readlines()
lines = [x.strip() for x in content]

if (len(content) != 2):
    print "Wrong number of lines!"
    printusage()
    exit()

# Get number of values and the values from the text file
numVals = lines.pop(0)
vals = lines.pop(0).split()
valsNew = []

for val in vals:
    # It's a float
    if regex.match(val):
        valsNew.extend(convertFloat(float(val)))

    else:
        valsNew.append(int(val))

# Now lets write it out as gfp_vals.in
f = open(fnameGfp, 'w')
f.write(str(len(valsNew)))
f.write('\n')
f.write(' '.join(map(str, valsNew)))
f.write('\n')
f.close

# Call gen_input_fp.x to generate gfp_vals.out
call(["./gen_input_fp.x"])
print "... gfp_vals.out written"
