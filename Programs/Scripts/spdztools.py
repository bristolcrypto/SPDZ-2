import sys
from ast import literal_eval

#str is a string "(v,p,z,s)"
def convertToFloat(str):
    input = literal_eval(str.strip())
    return float(input[0])*float(pow(2,(input[1])))*float(pow((-1),(input[3])))