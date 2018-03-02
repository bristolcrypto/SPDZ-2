# (C) 2018 University of Bristol. See License.txt

""" This module is for classes of actual assembly instructions.

All base classes, utility functions etc. should go in
instructions_base.py instead. This is for two reasons:
1) Easier generation of documentation
2) Ensures that 'from instructions import *' will only import assembly
instructions and nothing else.

Note: every instruction should have a suitable docstring for auto-generation of
documentation
"""

import itertools
import tools
from random import randint
from Compiler.config import *
from Compiler.exceptions import *
import Compiler.instructions_base as base


# avoid naming collision with input instruction
_python_input = input

###
### Load and store instructions
###

@base.gf2n
@base.vectorize
class ldi(base.Instruction):
    r""" Assigns register $c_i$ the value $n$. """
    __slots__ = []
    code = base.opcodes['LDI']
    arg_format = ['cw','i']
    
    def execute(self):
        self.args[0].value = self.args[1]

@base.gf2n
@base.vectorize
class ldsi(base.Instruction):
    r""" Assigns register $s_i$ a share of the value $n$. """
    __slots__ = []
    code = base.opcodes['LDSI']
    arg_format = ['sw','i']
    
    def execute(self):
        self.args[0].value = self.args[1]

@base.gf2n
@base.vectorize
class ldmc(base.DirectMemoryInstruction, base.ReadMemoryInstruction):
    r""" Assigns register $c_i$ the value in memory \verb+C[n]+. """
    __slots__ = ["code"]
    code = base.opcodes['LDMC']
    arg_format = ['cw','int']

    def execute(self):
        self.args[0].value = program.mem_c[self.args[1]]

@base.gf2n
@base.vectorize
class ldms(base.DirectMemoryInstruction, base.ReadMemoryInstruction):
    r""" Assigns register $s_i$ the value in memory \verb+S[n]+. """
    __slots__ = ["code"]
    code = base.opcodes['LDMS']
    arg_format = ['sw','int']

    def execute(self):
        self.args[0].value = program.mem_s[self.args[1]]

@base.gf2n
@base.vectorize
class stmc(base.DirectMemoryWriteInstruction):
    r""" Sets \verb+C[n]+ to be the value $c_i$. """
    __slots__ = ["code"]
    code = base.opcodes['STMC']
    arg_format = ['c','int']

    def execute(self):
        program.mem_c[self.args[1]] = self.args[0].value

@base.gf2n
@base.vectorize
class stms(base.DirectMemoryWriteInstruction):
    r""" Sets \verb+S[n]+ to be the value $s_i$. """
    __slots__ = ["code"]
    code = base.opcodes['STMS']
    arg_format = ['s','int']

    def execute(self):
        program.mem_s[self.args[1]] = self.args[0].value

@base.vectorize
class ldmint(base.DirectMemoryInstruction, base.ReadMemoryInstruction):
    r""" Assigns register $ci_i$ the value in memory \verb+Ci[n]+. """
    __slots__ = ["code"]
    code = base.opcodes['LDMINT']
    arg_format = ['ciw','int']

    def execute(self):
        self.args[0].value = program.mem_i[self.args[1]]

@base.vectorize
class stmint(base.DirectMemoryWriteInstruction):
    r""" Sets \verb+Ci[n]+ to be the value $ci_i$. """
    __slots__ = ["code"]
    code = base.opcodes['STMINT']
    arg_format = ['ci','int']

    def execute(self):
        program.mem_i[self.args[1]] = self.args[0].value

# must have seperate instructions because address is always modp
@base.vectorize
class ldmci(base.ReadMemoryInstruction):
    r""" Assigns register $c_i$ the value in memory \verb+C[cj]+. """
    code = base.opcodes['LDMCI']
    arg_format = ['cw','ci']
    
    def execute(self):
        self.args[0].value = program.mem_c[self.args[1].value]

@base.vectorize
class ldmsi(base.ReadMemoryInstruction):
    r""" Assigns register $s_i$ the value in memory \verb+S[cj]+. """
    code = base.opcodes['LDMSI']
    arg_format = ['sw','ci']

    def execute(self):
        self.args[0].value = program.mem_s[self.args[1].value]

@base.vectorize
class stmci(base.WriteMemoryInstruction):
    r""" Sets \verb+C[cj]+ to be the value $c_i$. """
    code = base.opcodes['STMCI']
    arg_format = ['c','ci']

    def execute(self):
        program.mem_c[self.args[1].value] = self.args[0].value

@base.vectorize
class stmsi(base.WriteMemoryInstruction):
    r""" Sets \verb+S[cj]+ to be the value $s_i$. """
    code = base.opcodes['STMSI']
    arg_format = ['s','ci']

    def execute(self):
        program.mem_s[self.args[1].value] = self.args[0].value

@base.vectorize
class ldminti(base.ReadMemoryInstruction):
    r""" Assigns register $ci_i$ the value in memory \verb+Ci[cj]+. """
    code = base.opcodes['LDMINTI']
    arg_format = ['ciw','ci']

    def execute(self):
        self.args[0].value = program.mem_i[self.args[1].value]

@base.vectorize
class stminti(base.WriteMemoryInstruction):
    r""" Sets \verb+Ci[cj]+ to be the value $ci_i$. """
    code = base.opcodes['STMINTI']
    arg_format = ['ci','ci']

    def execute(self):
        program.mem_i[self.args[1].value] = self.args[0].value

@base.vectorize
class gldmci(base.ReadMemoryInstruction):
    r""" Assigns register $c_i$ the value in memory \verb+C[cj]+. """
    code = base.opcodes['LDMCI'] + 0x100
    arg_format = ['cgw','ci']
    
    def execute(self):
        self.args[0].value = program.mem_c[self.args[1].value]

@base.vectorize
class gldmsi(base.ReadMemoryInstruction):
    r""" Assigns register $s_i$ the value in memory \verb+S[cj]+. """
    code = base.opcodes['LDMSI'] + 0x100
    arg_format = ['sgw','ci']

    def execute(self):
        self.args[0].value = program.mem_s[self.args[1].value]

@base.vectorize
class gstmci(base.WriteMemoryInstruction):
    r""" Sets \verb+C[cj]+ to be the value $c_i$. """
    code = base.opcodes['STMCI'] + 0x100
    arg_format = ['cg','ci']

    def execute(self):
        program.mem_c[self.args[1].value] = self.args[0].value

@base.vectorize
class gstmsi(base.WriteMemoryInstruction):
    r""" Sets \verb+S[cj]+ to be the value $s_i$. """
    code = base.opcodes['STMSI'] + 0x100
    arg_format = ['sg','ci']

    def execute(self):
        program.mem_s[self.args[1].value] = self.args[0].value

@base.gf2n
@base.vectorize
class protectmems(base.Instruction):
    r""" Protects secret memory range $[ci_i,ci_j)$. """
    code = base.opcodes['PROTECTMEMS']
    arg_format = ['ci','ci']

@base.gf2n
@base.vectorize
class protectmemc(base.Instruction):
    r""" Protects clear memory range $[ci_i,ci_j)$. """
    code = base.opcodes['PROTECTMEMC']
    arg_format = ['ci','ci']

@base.gf2n
@base.vectorize
class protectmemint(base.Instruction):
    r""" Protects integer memory range $[ci_i,ci_j)$. """
    code = base.opcodes['PROTECTMEMINT']
    arg_format = ['ci','ci']

@base.gf2n
@base.vectorize
class movc(base.Instruction):
    r""" Assigns register $c_i$ the value in the register $c_j$. """
    __slots__ = ["code"]
    code = base.opcodes['MOVC']
    arg_format = ['cw','c']

    def execute(self):
        self.args[0].value = self.args[1].value

@base.gf2n
@base.vectorize
class movs(base.Instruction):
    r""" Assigns register $s_i$ the value in the register $s_j$. """
    __slots__ = ["code"]
    code = base.opcodes['MOVS']
    arg_format = ['sw','s']

    def execute(self):
        self.args[0].value = self.args[1].value

@base.vectorize
class movint(base.Instruction):
    r""" Assigns register $ci_i$ the value in the register $ci_j$. """
    __slots__ = ["code"]
    code = base.opcodes['MOVINT']
    arg_format = ['ciw','ci']

@base.vectorize
class pushint(base.StackInstruction):
    r""" Pushes register $ci_i$ to the thread-local stack. """
    code = base.opcodes['PUSHINT']
    arg_format = ['ci']

@base.vectorize
class popint(base.StackInstruction):
    r""" Pops from the thread-local stack to register $ci_i$. """
    code = base.opcodes['POPINT']
    arg_format = ['ciw']


###
### Machine
###

@base.vectorize
class ldtn(base.Instruction):
    r""" Assigns register $c_i$ the number of the current thread. """
    code = base.opcodes['LDTN']
    arg_format = ['ciw']

@base.vectorize
class ldarg(base.Instruction):
    r""" Assigns register $c_i$ the argument passed to the current thread. """
    code = base.opcodes['LDARG']
    arg_format = ['ciw']

@base.vectorize
class starg(base.Instruction):
    r""" Assigns register $c_i$ to the argument. """
    code = base.opcodes['STARG']
    arg_format = ['ci']

@base.gf2n
class reqbl(base.Instruction):
    r""" Require bit length $n". """
    code = base.opcodes['REQBL']
    arg_format = ['int']

class time(base.Instruction):
    r""" Output epoch time. """
    code = base.opcodes['TIME']
    arg_format = []

class start(base.Instruction):
    r""" Start timer. """
    code = base.opcodes['START']
    arg_format = ['i']

class stop(base.Instruction):
    r""" Stop timer. """
    code = base.opcodes['STOP']
    arg_format = ['i']

class use(base.Instruction):
    r""" Offline data usage. """
    code = base.opcodes['USE']
    arg_format = ['int','int','int']

class use_inp(base.Instruction):
    r""" Input usage. """
    code = base.opcodes['USE_INP']
    arg_format = ['int','int','int']

class run_tape(base.Instruction):
    r""" Start tape $n$ in thread $c_i$ with argument $c_j$. """
    code = base.opcodes['RUN_TAPE']
    arg_format = ['int','int','int']

class join_tape(base.Instruction):
    r""" Join thread $c_i$. """
    code = base.opcodes['JOIN_TAPE']
    arg_format = ['int']

class crash(base.IOInstruction):
    r""" Crash runtime. """
    code = base.opcodes['CRASH']
    arg_format = []

@base.gf2n
class use_prep(base.Instruction):
    r""" Input usage. """
    code = base.opcodes['USE_PREP']
    arg_format = ['str','int']

###
### Basic arithmetic
###

@base.gf2n
@base.vectorize
class addc(base.AddBase):
    r""" Clear addition $c_i=c_j+c_k$. """
    __slots__ = []
    code = base.opcodes['ADDC']
    arg_format = ['cw','c','c']

@base.gf2n
@base.vectorize
class adds(base.AddBase):
    r""" Secret addition $s_i=s_j+s_k$. """
    __slots__ = []
    code = base.opcodes['ADDS']
    arg_format = ['sw','s','s']

@base.gf2n
@base.vectorize
class addm(base.AddBase):
    r""" Mixed addition $s_i=s_j+c_k$. """
    __slots__ = []
    code = base.opcodes['ADDM']
    arg_format = ['sw','s','c']

@base.gf2n
@base.vectorize
class subc(base.SubBase):
    r""" Clear subtraction $c_i=c_j-c_k$. """
    __slots__ = []
    code = base.opcodes['SUBC']
    arg_format = ['cw','c','c']

@base.gf2n
@base.vectorize
class subs(base.SubBase):
    r""" Secret subtraction $s_i=s_j-s_k$. """
    __slots__ = []
    code = base.opcodes['SUBS']
    arg_format = ['sw','s','s']

@base.gf2n
@base.vectorize
class subml(base.SubBase):
    r""" Mixed subtraction $s_i=s_j-c_k$. """
    __slots__ = []
    code = base.opcodes['SUBML']
    arg_format = ['sw','s','c']

@base.gf2n
@base.vectorize
class submr(base.SubBase):
    r""" Mixed subtraction $s_i=c_j-s_k$. """
    __slots__ = []
    code = base.opcodes['SUBMR']
    arg_format = ['sw','c','s']

@base.gf2n
@base.vectorize
class mulc(base.MulBase):
    r""" Clear multiplication $c_i=c_j \cdot c_k$. """
    __slots__ = []
    code = base.opcodes['MULC']
    arg_format = ['cw','c','c']

@base.gf2n
@base.vectorize
class mulm(base.MulBase):
    r""" Mixed multiplication $s_i=c_j \cdot s_k$. """
    __slots__ = []
    code = base.opcodes['MULM']
    arg_format = ['sw','s','c']

@base.gf2n
@base.vectorize
class divc(base.Instruction):
    r""" Clear division $c_i=c_j/c_k$. """
    __slots__ = []
    code = base.opcodes['DIVC']
    arg_format = ['cw','c','c']
    
    def execute(self):
        self.args[0].value = self.args[1].value * pow(self.args[2].value, program.P-2, program.P) % program.P

@base.gf2n
@base.vectorize
class modc(base.Instruction):
    r""" Clear modular reduction $c_i=c_j/c_k$. """
    __slots__ = []
    code = base.opcodes['MODC']
    arg_format = ['cw','c','c']

    def execute(self):
        self.args[0].value = self.args[1].value % self.args[2].value

@base.vectorize
class legendrec(base.Instruction):
    r""" Clear Legendre symbol computation, $c_i = (c_j / p)$. """
    __slots__ = []
    code = base.opcodes['LEGENDREC']
    arg_format = ['cw','c']

@base.vectorize
class digestc(base.Instruction):
    r""" Clear truncated hash computation, $c_i = H(c_j)[bytes]$. """
    __slots__ = []
    code = base.opcodes['DIGESTC']
    arg_format = ['cw','c','int']

###
### Bitwise operations
###

@base.gf2n
@base.vectorize
class andc(base.Instruction):
    r""" Clear logical AND $c_i = c_j \land c_k$ """
    __slots__ = []
    code = base.opcodes['ANDC']
    arg_format = ['cw','c','c']
    
    def execute(self):
        self.args[0].value = (self.args[1].value & self.args[2].value) % program.P

@base.gf2n
@base.vectorize
class orc(base.Instruction):
    r""" Clear logical OR $c_i = c_j \lor c_k$ """
    __slots__ = []
    code = base.opcodes['ORC']
    arg_format = ['cw','c','c']
    
    def execute(self):
        self.args[0].value = (self.args[1].value | self.args[2].value) % program.P

@base.gf2n
@base.vectorize
class xorc(base.Instruction):
    r""" Clear logical XOR $c_i = c_j \oplus c_k$ """
    __slots__ = []
    code = base.opcodes['XORC']
    arg_format = ['cw','c','c']
    
    def execute(self):
        self.args[0].value = (self.args[1].value ^ self.args[2].value) % program.P

@base.vectorize
class notc(base.Instruction):
    r""" Clear logical NOT $c_i = \lnot c_j$ """
    __slots__ = []
    code = base.opcodes['NOTC']
    arg_format = ['cw','c', 'int']
    
    def execute(self):
        self.args[0].value = (~self.args[1].value + 2 ** self.args[2]) % program.P

@base.vectorize
class gnotc(base.Instruction):
    r""" Clear logical NOT $cg_i = \lnot cg_j$ """
    __slots__ = []
    code = (1 << 8) + base.opcodes['NOTC']
    arg_format = ['cgw','cg']

    def is_gf2n(self):
        return True

    def execute(self):
        self.args[0].value = ~self.args[1].value

@base.vectorize
class gbitdec(base.Instruction):
    r""" Store every $n$-th bit of $cg_i$ in $cg_j, \dots$. """
    __slots__ = []
    code = base.opcodes['GBITDEC']
    arg_format = tools.chain(['cg', 'int'], itertools.repeat('cgw'))

    def is_g2fn(self):
        return True

    def has_var_args(self):
        return True

@base.vectorize
class gbitcom(base.Instruction):
    r""" Store the bits $cg_j, \dots$ as every $n$-th bit of $cg_i$. """
    __slots__ = []
    code = base.opcodes['GBITCOM']
    arg_format = tools.chain(['cgw', 'int'], itertools.repeat('cg'))

    def is_g2fn(self):
        return True

    def has_var_args(self):
        return True


###
### Special GF(2) arithmetic instructions
###

@base.vectorize
class gmulbitc(base.MulBase):
    r""" Clear GF(2^n) by clear GF(2) multiplication """
    __slots__ = []
    code = base.opcodes['GMULBITC']
    arg_format = ['cgw','cg','cg']

    def is_gf2n(self):
        return True

@base.vectorize
class gmulbitm(base.MulBase):
    r""" Secret GF(2^n) by clear GF(2) multiplication """
    __slots__ = []
    code = base.opcodes['GMULBITM']
    arg_format = ['sgw','sg','cg']

    def is_gf2n(self):
        return True

###
### Arithmetic with immediate values
###

@base.gf2n
@base.vectorize
class addci(base.ClearImmediate):
    """ Clear addition of immediate value $c_i=c_j+n$. """
    __slots__ = []
    code = base.opcodes['ADDCI']
    op = '__add__'

@base.gf2n
@base.vectorize
class addsi(base.SharedImmediate):
    """ Secret addition of immediate value $s_i=s_j+n$. """
    __slots__ = []
    code = base.opcodes['ADDSI']
    op = '__add__'

@base.gf2n
@base.vectorize
class subci(base.ClearImmediate):
    r""" Clear subtraction of immediate value $c_i=c_j-n$. """
    __slots__ = []
    code = base.opcodes['SUBCI']
    op = '__sub__'

@base.gf2n
@base.vectorize
class subsi(base.SharedImmediate):
    r""" Secret subtraction of immediate value $s_i=s_j-n$. """
    __slots__ = []
    code = base.opcodes['SUBSI']
    op = '__sub__'

@base.gf2n
@base.vectorize
class subcfi(base.ClearImmediate):
    r""" Clear subtraction from immediate value $c_i=n-c_j$. """
    __slots__ = []
    code = base.opcodes['SUBCFI']
    op = '__rsub__'

@base.gf2n
@base.vectorize
class subsfi(base.SharedImmediate):
    r""" Secret subtraction from immediate value $s_i=n-s_j$. """
    __slots__ = []
    code = base.opcodes['SUBSFI']
    op = '__rsub__'

@base.gf2n
@base.vectorize
class mulci(base.ClearImmediate):
    r""" Clear multiplication by immediate value $c_i=c_j \cdot n$. """
    __slots__ = []
    code = base.opcodes['MULCI']
    op = '__mul__'

@base.gf2n
@base.vectorize
class mulsi(base.SharedImmediate):
    r""" Secret multiplication by immediate value $s_i=s_j \cdot n$. """
    __slots__ = []
    code = base.opcodes['MULSI']
    op = '__mul__'

@base.gf2n
@base.vectorize
class divci(base.ClearImmediate):
    r""" Clear division by immediate value $c_i=c_j/n$. """
    __slots__ = []
    code = base.opcodes['DIVCI']
    def execute(self):
        self.args[0].value = self.args[1].value * pow(self.args[2], program.P-2, program.P) % program.P

@base.gf2n
@base.vectorize
class modci(base.ClearImmediate):
    r""" Clear modular reduction by immediate value $c_i=c_j \mod{n}$. """
    __slots__ = []
    code = base.opcodes['MODCI']
    op = '__mod__'

@base.gf2n
@base.vectorize
class andci(base.ClearImmediate):
    r""" Clear logical AND with immediate value $c_i = c_j \land c_k$ """
    __slots__ = []
    code = base.opcodes['ANDCI']
    op = '__and__'

@base.gf2n
@base.vectorize
class xorci(base.ClearImmediate):
    r""" Clear logical XOR with immediate value $c_i = c_j \oplus c_k$ """
    __slots__ = []
    code = base.opcodes['XORCI']
    op = '__xor__'

@base.gf2n
@base.vectorize
class orci(base.ClearImmediate):
    r""" Clear logical OR with immediate value $c_i = c_j \vee c_k$ """
    __slots__ = []
    code = base.opcodes['ORCI']
    op = '__or__'


###
### Shift instructions
###

@base.gf2n
@base.vectorize
class shlc(base.Instruction):
    r""" Clear bitwise shift left $c_i = c_j << c_k$ """
    __slots__ = []
    code = base.opcodes['SHLC']
    arg_format = ['cw','c','c']
    
    def execute(self):
        self.args[0].value = (self.args[1].value << self.args[2].value) % program.P

@base.gf2n
@base.vectorize
class shrc(base.Instruction):
    r""" Clear bitwise shift right $c_i = c_j >> c_k$ """
    __slots__ = []
    code = base.opcodes['SHRC']
    arg_format = ['cw','c','c']
    
    def execute(self):
        self.args[0].value = (self.args[1].value >> self.args[2].value) % program.P

@base.gf2n
@base.vectorize
class shlci(base.ClearShiftInstruction):
    r""" Clear bitwise shift left by immediate value $c_i = c_j << n$ """
    __slots__ = []
    code = base.opcodes['SHLCI']
    op = '__lshift__'

@base.gf2n
@base.vectorize
class shrci(base.ClearShiftInstruction):
    r""" Clear bitwise shift right by immediate value $c_i = c_j >> n$ """
    __slots__ = []
    code = base.opcodes['SHRCI']
    op = '__rshift__'


###
### Data access instructions
###

@base.gf2n
@base.vectorize
class triple(base.DataInstruction):
    r""" Load secret variables $s_i$, $s_j$ and $s_k$
    with the next multiplication triple. """
    __slots__ = ['data_type']
    code = base.opcodes['TRIPLE']
    arg_format = ['sw','sw','sw']
    data_type = 'triple'
    
    def execute(self):
        self.args[0].value = randint(0,program.P)
        self.args[1].value = randint(0,program.P)
        self.args[2].value = (self.args[0].value * self.args[1].value) % program.P

@base.vectorize
class gbittriple(base.DataInstruction):
    r""" Load secret variables $s_i$, $s_j$ and $s_k$
    with the next GF(2) multiplication triple. """
    __slots__ = ['data_type']
    code = base.opcodes['GBITTRIPLE']
    arg_format = ['sgw','sgw','sgw']
    data_type = 'bittriple'
    field_type = 'gf2n'

    def is_gf2n(self):
        return True

@base.vectorize
class gbitgf2ntriple(base.DataInstruction):
    r""" Load secret variables $s_i$, $s_j$ and $s_k$
    with the next GF(2) and GF(2^n) multiplication triple. """
    code = base.opcodes['GBITGF2NTRIPLE']
    arg_format = ['sgw','sgw','sgw']
    data_type = 'bitgf2ntriple'
    field_type = 'gf2n'

    def is_gf2n(self):
        return True

@base.gf2n
@base.vectorize
class bit(base.DataInstruction):
    r""" Load secret variable $s_i$
    with the next secret bit. """
    __slots__ = []
    code = base.opcodes['BIT']
    arg_format = ['sw']
    data_type = 'bit'
    
    def execute(self):
        self.args[0].value = randint(0,1)

@base.gf2n
@base.vectorize
class square(base.DataInstruction):
    r""" Load secret variables $s_i$ and $s_j$
    with the next squaring tuple. """
    __slots__ = []
    code = base.opcodes['SQUARE']
    arg_format = ['sw','sw']
    data_type = 'square'
    
    def execute(self):
        self.args[0].value = randint(0,program.P)
        self.args[1].value = (self.args[0].value * self.args[0].value) % program.P

@base.gf2n
@base.vectorize
class inverse(base.DataInstruction):
    r""" Load secret variables $s_i$, $s_j$ and $s_k$
    with the next inverse triple. """
    __slots__ = []
    code = base.opcodes['INV']
    arg_format = ['sw','sw']
    data_type = 'inverse'
    
    def execute(self):
        self.args[0].value = randint(0,program.P)
        import gmpy
        self.args[1].value = int(gmpy.invert(self.args[0].value, program.P))

@base.gf2n
@base.vectorize
class inputmask(base.Instruction):
    r""" Load secret $s_i$ with the next input mask for player $p$ and
    write the mask on player $p$'s private output. """ 
    __slots__ = []
    code = base.opcodes['INPUTMASK']
    arg_format = ['sw', 'p']
    field_type = 'modp'

    def add_usage(self, req_node):
        req_node.increment((self.field_type, 'input', self.args[1]), \
                               self.get_size())

@base.gf2n
@base.vectorize
class prep(base.Instruction):
    r""" Custom preprocessed data """
    __slots__ = []
    code = base.opcodes['PREP']
    arg_format = tools.chain(['str'], itertools.repeat('sw'))
    gf2n_arg_format = tools.chain(['str'], itertools.repeat('sgw'))
    field_type = 'modp'

    def add_usage(self, req_node):
        req_node.increment((self.field_type, self.args[0]), 1)

    def has_var_args(self):
        return True

###
### I/O
###

@base.gf2n
@base.vectorize
class asm_input(base.IOInstruction):
    r""" Receive input from player $p$ and put in register $s_i$. """
    __slots__ = []
    code = base.opcodes['INPUT']
    arg_format = ['sw', 'p']
    field_type = 'modp'

    def add_usage(self, req_node):
        req_node.increment((self.field_type, 'input', self.args[1]), \
                               self.get_size())
    def execute(self):
        self.args[0].value = _python_input("Enter player %d's input:" % self.args[1]) % program.P

@base.gf2n
class startinput(base.RawInputInstruction):
    r""" Receive inputs from player $p$. """
    __slots__ = []
    code = base.opcodes['STARTINPUT']
    arg_format = ['p', 'int']
    field_type = 'modp'

    def add_usage(self, req_node):
        req_node.increment((self.field_type, 'input', self.args[0]), \
                               self.args[1])

class stopinput(base.RawInputInstruction):
    r""" Receive inputs from player $p$ and put in registers. """
    __slots__ = []
    code = base.opcodes['STOPINPUT']
    arg_format = tools.chain(['p'], itertools.repeat('sw'))

    def has_var_args(self):
        return True

class gstopinput(base.RawInputInstruction):
    r""" Receive inputs from player $p$ and put in registers. """
    __slots__ = []
    code = 0x100 + base.opcodes['STOPINPUT']
    arg_format = tools.chain(['p'], itertools.repeat('sgw'))

    def has_var_args(self):
        return True

@base.gf2n
@base.vectorize
class print_mem(base.IOInstruction):
    r""" Print value in clear memory \verb|C[ci]| to stdout. """
    __slots__ = []
    code = base.opcodes['PRINTMEM']
    arg_format = ['c']
    
    def execute(self):
        pass

@base.gf2n
@base.vectorize
class print_reg(base.IOInstruction):
    r""" Print value of register \verb|ci| to stdout and optional 4-char comment. """
    __slots__ = []
    code = base.opcodes['PRINTREG']
    arg_format = ['c','i']
    
    def __init__(self, reg, comment=''):
        super(print_reg_class, self).__init__(reg, self.str_to_int(comment))

    def execute(self):
        pass

@base.gf2n
@base.vectorize
class print_reg_plain(base.IOInstruction):
    r""" Print only the value of register \verb|ci| to stdout. """
    __slots__ = []
    code = base.opcodes['PRINTREGPLAIN']
    arg_format = ['c']


@base.vectorize
class print_float_plain(base.IOInstruction):
    __slots__ = []
    code = base.opcodes['PRINTFLOATPLAIN']
    arg_format = ['c', 'c', 'c', 'c']

class print_int(base.IOInstruction):
    r""" Print only the value of register \verb|ci| to stdout. """
    __slots__ = []
    code = base.opcodes['PRINTINT']
    arg_format = ['ci']

class print_char(base.IOInstruction):
    r""" Print a single character to stdout. """
    code = base.opcodes['PRINTCHR']
    arg_format = ['int']

    def __init__(self, ch):
        super(print_char, self).__init__(ord(ch))

class print_char4(base.IOInstruction):
    r""" Print a 4 character string. """
    code = base.opcodes['PRINTSTR']
    arg_format = ['int']

    def __init__(self, val):
        super(print_char4, self).__init__(self.str_to_int(val))

@base.vectorize
class print_char_regint(base.IOInstruction):
    r""" Print register $ci_i$ as a single character to stdout. """
    code = base.opcodes['PRINTCHRINT']
    arg_format = ['ci']

@base.vectorize
class print_char4_regint(base.IOInstruction):
    r""" Print register $ci_i$ as a four character string to stdout. """
    code = base.opcodes['PRINTSTRINT']
    arg_format = ['ci']

@base.vectorize
class pubinput(base.PublicFileIOInstruction):
    __slots__ = []
    code = base.opcodes['PUBINPUT']
    arg_format = ['ciw']

@base.vectorize
class readsocketc(base.IOInstruction):
    """Read a variable number of clear GF(p) values from socket for a specified client id and store in registers"""
    __slots__ = []
    code = base.opcodes['READSOCKETC']
    arg_format = tools.chain(['ci'], itertools.repeat('cw'))

    def has_var_args(self):
        return True

@base.vectorize
class readsockets(base.IOInstruction):
    """Read a variable number of secret shares + MACs from socket for a client id and store in registers"""
    __slots__ = []
    code = base.opcodes['READSOCKETS']
    arg_format = tools.chain(['ci'], itertools.repeat('sw'))

    def has_var_args(self):
        return True

@base.vectorize
class readsocketint(base.IOInstruction):
    """Read variable number of 32-bit int from socket for a client id and store in registers"""
    __slots__ = []
    code = base.opcodes['READSOCKETINT']
    arg_format = tools.chain(['ci'], itertools.repeat('ciw'))

    def has_var_args(self):
        return True

@base.vectorize
class writesocketc(base.IOInstruction):
    """
    Write a variable number of clear GF(p) values from registers into socket 
    for a specified client id, message_type
    """
    __slots__ = []
    code = base.opcodes['WRITESOCKETC']
    arg_format = tools.chain(['ci', 'int'], itertools.repeat('c'))

    def has_var_args(self):
        return True

@base.vectorize
class writesockets(base.IOInstruction):
    """
    Write a variable number of secret shares + MACs from registers into a socket
    for a specified client id, message_type
    """
    __slots__ = []
    code = base.opcodes['WRITESOCKETS']
    arg_format = tools.chain(['ci', 'int'], itertools.repeat('s'))

    def has_var_args(self):
        return True

@base.vectorize
class writesocketshare(base.IOInstruction):
    """
    Write a variable number of secret shares (without MACs) from registers into socket 
    for a specified client id, message_type
    """
    __slots__ = []
    code = base.opcodes['WRITESOCKETSHARE']
    arg_format = tools.chain(['ci', 'int'], itertools.repeat('s'))

    def has_var_args(self):
        return True

@base.vectorize
class writesocketint(base.IOInstruction):
    """
    Write a variable number of 32-bit ints from registers into socket
    for a specified client id, message_type
    """
    __slots__ = []
    code = base.opcodes['WRITESOCKETINT']
    arg_format = tools.chain(['ci', 'int'], itertools.repeat('ci'))

    def has_var_args(self):
        return True

class listen(base.IOInstruction):
    """Open a server socket on a party specific port number and listen for client connections (non-blocking)"""
    __slots__ = []
    code = base.opcodes['LISTEN']
    arg_format = ['int']

class acceptclientconnection(base.IOInstruction):
    """Wait for a connection at the given port and write socket handle to register """
    __slots__ = []
    code = base.opcodes['ACCEPTCLIENTCONNECTION']
    arg_format = ['ciw', 'int']

class connectipv4(base.IOInstruction):
    """Connect to server at IPv4 address in register \verb|cj| at given port. Write socket handle to register \verb|ci|"""
    __slots__ = []
    code = base.opcodes['CONNECTIPV4']
    arg_format = ['ciw', 'ci', 'int']

class readclientpublickey(base.IOInstruction):
    """Read a client public key as 8 32-bit ints for a specified client id"""
    __slots__ = []
    code = base.opcodes['READCLIENTPUBLICKEY']
    arg_format = tools.chain(['ci'], itertools.repeat('ci'))

    def has_var_args(self):
        return True

class initsecuresocket(base.IOInstruction):
    """Read a client public key as 8 32-bit ints for a specified client id,
    negotiate a shared key via STS and use it for replay resistant comms"""
    __slots__ = []
    code = base.opcodes['INITSECURESOCKET']
    arg_format = tools.chain(['ci'], itertools.repeat('ci'))

    def has_var_args(self):
        return True

class respsecuresocket(base.IOInstruction):
    """Read a client public key as 8 32-bit ints for a specified client id,
    negotiate a shared key via STS and use it for replay resistant comms"""
    __slots__ = []
    code = base.opcodes['RESPSECURESOCKET']
    arg_format = tools.chain(['ci'], itertools.repeat('ci'))

    def has_var_args(self):
        return True

class writesharestofile(base.IOInstruction):
    """Write shares to a file"""
    __slots__ = []
    code = base.opcodes['WRITEFILESHARE']
    arg_format = itertools.repeat('s')

    def has_var_args(self):
        return True

class readsharesfromfile(base.IOInstruction):
    """
    Read shares from a file. Pass in start posn, return finish posn, shares.
    Finish posn will return:
      -2 file not found
      -1 eof reached
      position in file after read finished
    """
    __slots__ = []
    code = base.opcodes['READFILESHARE']
    arg_format = tools.chain(['ci', 'ciw'], itertools.repeat('sw'))

    def has_var_args(self):
        return True

@base.gf2n
@base.vectorize
class raw_output(base.PublicFileIOInstruction):
    r""" Raw output of register \verb|ci| to file. """
    __slots__ = []
    code = base.opcodes['RAWOUTPUT']
    arg_format = ['c']

@base.gf2n
@base.vectorize
class startprivateoutput(base.Instruction):
    r""" Initiate private output to $n$ of $s_j$ via $s_i$. """
    __slots__ = []
    code = base.opcodes['STARTPRIVATEOUTPUT']
    arg_format = ['sw','s','p']

@base.gf2n
@base.vectorize
class stopprivateoutput(base.Instruction):
    r""" Previously iniated private output to $n$ via $c_i$. """
    __slots__ = []
    code = base.opcodes['STOPPRIVATEOUTPUT']
    arg_format = ['c','p']

@base.vectorize
class rand(base.Instruction):
    __slots__ = []
    code = base.opcodes['RAND']
    arg_format = ['ciw','ci']

###
### Integer operations
### 

@base.vectorize
class ldint(base.Instruction):
    __slots__ = []
    code = base.opcodes['LDINT']
    arg_format = ['ciw', 'i']

@base.vectorize
class addint(base.IntegerInstruction):
    __slots__ = []
    code = base.opcodes['ADDINT']

@base.vectorize
class subint(base.IntegerInstruction):
    __slots__ = []
    code = base.opcodes['SUBINT']

@base.vectorize
class mulint(base.IntegerInstruction):
    __slots__ = []
    code = base.opcodes['MULINT']

@base.vectorize
class divint(base.IntegerInstruction):
    __slots__ = []
    code = base.opcodes['DIVINT']

###
### Clear comparison instructions
###

@base.vectorize
class eqzc(base.UnaryComparisonInstruction):
    r""" Clear comparison $c_i = (c_j \stackrel{?}{==} 0)$. """
    __slots__ = []
    code = base.opcodes['EQZC']
    
    def execute(self):
        if self.args[1].value == 0:
            self.args[0].value = 1
        else:
            self.args[0].value = 0

@base.vectorize
class ltzc(base.UnaryComparisonInstruction):
    r""" Clear comparison $c_i = (c_j \stackrel{?}{<} 0)$. """
    __slots__ = []
    code = base.opcodes['LTZC']

@base.vectorize
class ltc(base.IntegerInstruction):
    r""" Clear comparison $c_i = (c_j \stackrel{?}{<} c_k)$. """
    __slots__ = []
    code = base.opcodes['LTC']

@base.vectorize
class gtc(base.IntegerInstruction):
    r""" Clear comparison $c_i = (c_j \stackrel{?}{>} c_k)$. """
    __slots__ = []
    code = base.opcodes['GTC']

@base.vectorize
class eqc(base.IntegerInstruction):
    r""" Clear comparison $c_i = (c_j \stackrel{?}{==} c_k)$. """
    __slots__ = []
    code = base.opcodes['EQC']


###
### Jumps etc
###

class jmp(base.JumpInstruction):
    """ Unconditional relative jump of $n+1$ instructions. """
    __slots__ = []
    code = base.opcodes['JMP']
    arg_format = ['int']
    jump_arg = 0

    def execute(self):
        pass

class jmpi(base.JumpInstruction):
    """ Unconditional relative jump of $c_i+1$ instructions. """
    __slots__ = []
    code = base.opcodes['JMPI']
    arg_format = ['ci']
    jump_arg = 0

class jmpnz(base.JumpInstruction):
    r""" Jump $n+1$ instructions if $c_i \neq 0$.

    e.g.
    jmpnz(c, n) : advance n+1 instructions if c is non-zero 
    jmpnz(c, 0) : do nothing
    jmpnz(c, -1): infinite loop if c is non-zero
    """
    __slots__ = []
    code = base.opcodes['JMPNZ']
    arg_format = ['ci', 'int']
    jump_arg = 1
    
    def execute(self):
        pass

class jmpeqz(base.JumpInstruction):
    r""" Jump $n+1$ instructions if $c_i == 0$. """
    __slots__ = []
    code = base.opcodes['JMPEQZ']
    arg_format = ['ci', 'int']
    jump_arg = 1
    
    def execute(self):
        pass

###
### Conversions
###

@base.gf2n
@base.vectorize
class convint(base.Instruction):
    """ Convert from integer register $ci_j$ to clear modp register $c_i$. """
    __slots__ =  []
    code = base.opcodes['CONVINT']
    arg_format = ['cw', 'ci']

@base.vectorize
class convmodp(base.Instruction):
    """ Convert from clear modp register $c_j$ to integer register $ci_i$. """
    __slots__ =  []
    code = base.opcodes['CONVMODP']
    arg_format = ['ciw', 'c', 'int']
    def __init__(self, *args, **kwargs):
        bitlength = kwargs.get('bitlength', program.bit_length)
        super(convmodp_class, self).__init__(*(args + (bitlength,)))

@base.vectorize
class gconvgf2n(base.Instruction):
    """ Convert from clear modp register $c_j$ to integer register $ci_i$. """
    __slots__ =  []
    code = base.opcodes['GCONVGF2N']
    arg_format = ['ciw', 'cg']

###
### Other instructions
###

@base.gf2n
@base.vectorize
class startopen(base.VarArgsInstruction):
    """ Start opening secret register $s_i$. """
    __slots__ = []
    code = base.opcodes['STARTOPEN']
    arg_format = itertools.repeat('s')
    
    def execute(self):
        for arg in self.args[::-1]:
            program.curr_block.open_queue.append(arg.value)

@base.gf2n
@base.vectorize
class stopopen(base.VarArgsInstruction):
    """ Store previous opened value in $c_i$. """
    __slots__ = []
    code = base.opcodes['STOPOPEN']
    arg_format = itertools.repeat('cw')
    
    def execute(self):
        for arg in self.args:
            arg.value = program.curr_block.open_queue.pop()

###
### CISC-style instructions
###

# rename 'open' to avoid conflict with built-in open function
@base.gf2n
@base.vectorize
class asm_open(base.CISC):
    """ Open the value in $s_j$ and assign it to $c_i$. """
    __slots__ = []
    arg_format = ['cw','s']
    
    def expand(self):
        startopen(self.args[1])
        stopopen(self.args[0])


@base.gf2n
@base.vectorize
class muls(base.CISC):
    """ Secret multiplication $s_i = s_j \cdot s_k$. """
    __slots__ = []
    arg_format = ['sw','s','s']
    
    def expand(self):
        s = [program.curr_block.new_reg('s') for i in range(9)]
        c = [program.curr_block.new_reg('c') for i in range(3)]
        triple(s[0], s[1], s[2])
        subs(s[3], self.args[1], s[0])
        subs(s[4], self.args[2], s[1])
        startopen(s[3], s[4])
        stopopen(c[0], c[1])
        mulm(s[5], s[1], c[0])
        mulm(s[6], s[0], c[1])
        mulc(c[2], c[0], c[1])
        adds(s[7], s[2], s[5])
        adds(s[8], s[7], s[6])
        addm(self.args[0], s[8], c[2])

@base.gf2n
@base.vectorize
class sqrs(base.CISC):
    """ Secret squaring $s_i = s_j \cdot s_j$. """
    __slots__ = []
    arg_format = ['sw', 's']
    
    def expand(self):
        s = [program.curr_block.new_reg('s') for i in range(6)]
        c = [program.curr_block.new_reg('c') for i in range(2)]
        square(s[0], s[1])
        subs(s[2], self.args[1], s[0])
        asm_open(c[0], s[2])
        mulc(c[1], c[0], c[0])
        mulm(s[3], self.args[1], c[0])
        adds(s[4], s[3], s[3])
        adds(s[5], s[1], s[4])
        subml(self.args[0], s[5], c[1])


@base.gf2n
@base.vectorize
class lts(base.CISC):
    """ Secret comparison $s_i = (s_j < s_k)$. """
    __slots__ = []
    arg_format = ['sw', 's', 's', 'int', 'int']

    def expand(self):
        a = program.curr_block.new_reg('s')
        subs(a, self.args[1], self.args[2])
        comparison.LTZ(self.args[0], a, self.args[3], self.args[4])

@base.vectorize
class g2muls(base.CISC):
    r""" Secret GF(2) multiplication """
    __slots__ = []
    arg_format = ['sgw','sg','sg']

    def expand(self):
        s = [program.curr_block.new_reg('sg') for i in range(9)]
        c = [program.curr_block.new_reg('cg') for i in range(3)]
        gbittriple(s[0], s[1], s[2])
        gsubs(s[3], self.args[1], s[0])
        gsubs(s[4], self.args[2], s[1])
        gstartopen(s[3], s[4])
        gstopopen(c[0], c[1])
        gmulbitm(s[5], s[1], c[0])
        gmulbitm(s[6], s[0], c[1])
        gmulbitc(c[2], c[0], c[1])
        gadds(s[7], s[2], s[5])
        gadds(s[8], s[7], s[6])
        gaddm(self.args[0], s[8], c[2])

#@base.vectorize
#class gmulbits(base.CISC):
#    r""" Secret $GF(2^n) \times GF(2)$ multiplication """
#    __slots__ = []
#    arg_format = ['sgw','sg','sg']
#
#    def expand(self):
#        s = [program.curr_block.new_reg('s') for i in range(9)]
#        c = [program.curr_block.new_reg('c') for i in range(3)]
#        g2ntriple(s[0], s[1], s[2])
#        subs(s[3], self.args[1], s[0])
#        subs(s[4], self.args[2], s[1])
#        startopen(s[3], s[4])
#        stopopen(c[0], c[1])
#        mulm(s[5], s[1], c[0])
#        mulm(s[6], s[0], c[1])
#        mulc(c[2], c[0], c[1])
#        adds(s[7], s[2], s[5])
#        adds(s[8], s[7], s[6])
#        addm(self.args[0], s[8], c[2])

# hack for circular dependency
from Compiler import comparison
