#!/usr/bin/python
import re
import math
import subprocess 

def printusage():
    print """ 
            Format of float_vals.in:
            - First line contains number of values
            - Second line contains a space separated list of integer or float values.
              
            E.g:
            3
            1.337 1338 3.14159
          """

fnameFloat = "float_vals.out"
fnameSint = "gfp_output_vals.out"

# Call gen_input_fp.x to generate gfp_vals.out
popen = subprocess.Popen("./gen_output_fp.x", stdout=subprocess.PIPE)
popen.wait()
output = popen.stdout.read()
print output

print "reading gfp_output_vals.out ..."
with open(fnameSint) as f:
    content = f.readlines()
lines = [x.strip() for x in content]

# Get number of values and the values from the text file
lines.pop(0)  # remove header
valsNew = []

while len(lines) >=4:
    v = lines.pop(0)
    p = lines.pop(0)
    z = lines.pop(0)
    s = lines.pop(0)
    valsNew.append((1 -2*int(s))*(1-int(z))*float(v)*(2**float(p)))

# Now lets write it out as gfp_vals.in
f = open(fnameFloat, 'w')
f.write("Output Values:")
f.write('\n')
f.write(' '.join(map(str, valsNew)))
f.write('\n')
f.close()

print "... float_vals.out"
