import random

# constructs an n-by-n matrix
def genRandomMatrix(n):
    for i in range(1, n+1):
        linestr = "A" + str(i) + " = " + "["
        for j in range(1, n+1):
            r = random.uniform(-10, 10)
            linestr = linestr + "sfloat(" + str(r) + "),"
        print linestr[:-1] + "]\n"

    linestr = "A = ["
    for i in range(1, n+1):
        linestr = linestr + "A" + str(i) + ","
    print linestr[:-1] + "]\n"

genRandomMatrix(10)