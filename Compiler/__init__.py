# (C) 2018 University of Bristol. See License.txt

import compilerLib, program, instructions, types, library, floatingpoint
import inspect
from config import *
from compilerLib import run


# add all instructions to the program VARS dictionary
compilerLib.VARS = {}
instr_classes = [t[1] for t in inspect.getmembers(instructions, inspect.isclass)]

instr_classes += [t[1] for t in inspect.getmembers(types, inspect.isclass)\
    if t[1].__module__ == types.__name__]

instr_classes += [t[1] for t in inspect.getmembers(library, inspect.isfunction)\
    if t[1].__module__ == library.__name__]

for op in instr_classes:
    compilerLib.VARS[op.__name__] = op

# add open and input separately due to name conflict
compilerLib.VARS['open'] = instructions.asm_open
compilerLib.VARS['vopen'] = instructions.vasm_open
compilerLib.VARS['gopen'] = instructions.gasm_open
compilerLib.VARS['vgopen'] = instructions.vgasm_open
compilerLib.VARS['input'] = instructions.asm_input
compilerLib.VARS['ginput'] = instructions.gasm_input

compilerLib.VARS['comparison'] = comparison
compilerLib.VARS['floatingpoint'] = floatingpoint
