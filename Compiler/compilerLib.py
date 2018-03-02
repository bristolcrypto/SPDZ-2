# (C) 2018 University of Bristol. See License.txt

from Compiler.program import Program
from Compiler.config import *
from Compiler.exceptions import *
import instructions, instructions_base, types, comparison, library

import random
import time
import sys


def run(args, options, param=-1, merge_opens=True, emulate=True, \
            reallocate=True, assemblymode=False, debug=False):
    """ Compile a file and output a Program object.
    
    If merge_opens is set to True, will attempt to merge any parallelisable open
    instructions. """
    
    prog = Program(args, options, param, assemblymode)
    instructions.program = prog
    instructions_base.program = prog
    types.program = prog
    comparison.program = prog
    prog.EMULATE = emulate
    prog.DEBUG = debug
    VARS['program'] = prog
    comparison.set_variant(options)
    
    print 'Compiling file', prog.infile
    
    # no longer needed, but may want to support assembly in future (?)
    if assemblymode:
        prog.restart_main_thread()
        for i in xrange(INIT_REG_MAX):
            VARS['c%d'%i] = prog.curr_block.new_reg('c')
            VARS['s%d'%i] = prog.curr_block.new_reg('s')
            VARS['cg%d'%i] = prog.curr_block.new_reg('cg')
            VARS['sg%d'%i] = prog.curr_block.new_reg('sg')
            if i % 10000000 == 0 and i > 0:
                print "Initialized %d register variables at" % i, time.asctime()
        
        # first pass determines how many assembler registers are used
        prog.FIRST_PASS = True
        execfile(prog.infile, VARS)
    
    if instructions_base.Instruction.count != 0:
        print 'instructions count', instructions_base.Instruction.count
        instructions_base.Instruction.count = 0
    prog.FIRST_PASS = False
    prog.reset_values()
    # make compiler modules directly accessible
    sys.path.insert(0, 'Compiler')
    # create the tapes
    execfile(prog.infile, VARS)
    
    # optimize the tapes
    for tape in prog.tapes:
        tape.optimize(options)
    
    # check program still does the same thing after optimizations
    if emulate:
        clearmem = list(prog.mem_c)
        sharedmem = list(prog.mem_s)
        prog.emulate()
        if prog.mem_c != clearmem or prog.mem_s != sharedmem:
            print 'Warning: emulated memory values changed after compiler optimization'
        #    raise CompilerError('Compiler optimization caused incorrect memory write.')
    
    if prog.main_thread_running:
        prog.update_req(prog.curr_tape)
    print 'Program requires:', repr(prog.req_num)
    print 'Cost:', prog.req_num.cost()
    print 'Memory size:', prog.allocated_mem

    # finalize the memory
    prog.finalize_memory()

    return prog
