# (C) 2016 University of Bristol. See License.txt

from Compiler.program import Tape
from Compiler.exceptions import *
from Compiler.instructions import *
from Compiler.instructions_base import *
from floatingpoint import two_power
import comparison, floatingpoint
import math
import util
import operator


class MPCThread(object):
    def __init__(self, target, name, args = [], runtime_arg = None):
        """ Create a thread from a callable object. """
        if not callable(target):
            raise CompilerError('Target %s for thread %s is not callable' % (target,name))
        self.name = name
        self.tape = Tape(program.name + '-' + name, program)
        self.target = target
        self.args = args
        self.runtime_arg = runtime_arg
        self.running = 0
    
    def start(self, runtime_arg = None):
        self.running += 1
        program.start_thread(self, runtime_arg or self.runtime_arg)
    
    def join(self):
        if not self.running:
            raise CompilerError('Thread %s is not running' % self.name)
        self.running -= 1
        program.stop_thread(self)


def vectorize(operation):
    def vectorized_operation(self, *args, **kwargs):
        if len(args):
            if (isinstance(args[0], Tape.Register) or isinstance(args[0], sfloat)) \
                    and args[0].size != self.size:
                raise CompilerError('Different vector sizes of operands')
        set_global_vector_size(self.size)
        res = operation(self, *args, **kwargs)
        reset_global_vector_size()
        return res
    return vectorized_operation

def vectorized_classmethod(function):
    def vectorized_function(cls, *args, **kwargs):
        size = None
        if 'size' in kwargs:
            size = kwargs.pop('size')
        if size:
            set_global_vector_size(size)
            res = function(cls, *args, **kwargs)
            reset_global_vector_size()
        else:
            res = function(cls, *args, **kwargs)
        return res
    return classmethod(vectorized_function)

def vectorize_init(function):
    def vectorized_init(*args, **kwargs):
        size = None
        if len(args) > 1 and (isinstance(args[1], Tape.Register) or \
                    isinstance(args[1], sfloat)):
            size = args[1].size
            if 'size' in kwargs and kwargs['size'] is not None \
                    and kwargs['size'] != size:
                raise CompilerError('Mismatch in vector size')
        if 'size' in kwargs and kwargs['size']:
            size = kwargs['size']
        if size is not None:
            set_global_vector_size(size)
            res = function(*args, **kwargs)
            reset_global_vector_size()
        else:
            res = function(*args, **kwargs)
        return res
    return vectorized_init

def set_instruction_type(operation):
    def instruction_typed_operation(self, *args, **kwargs):
        set_global_instruction_type(self.instruction_type)
        res = operation(self, *args, **kwargs)
        reset_global_instruction_type()
        return res
    return instruction_typed_operation

def read_mem_value(operation):
    def read_mem_operation(self, other, *args, **kwargs):
        if isinstance(other, MemValue):
            other = other.read()
        return operation(self, other, *args, **kwargs)
    return read_mem_operation


class _number(object):
    def square(self):
        return self * self

    def __add__(self, other):
        if other is 0 or other is 0L:
            return self
        else:
            return self.add(other)

    def __mul__(self, other):
        if other is 0 or other is 0L:
            return 0
        elif other is 1 or other is 1L:
            return self
        else:
            return self.mul(other)

    __radd__ = __add__
    __rmul__ = __mul__

    @vectorize
    def __pow__(self, exp):
        if isinstance(exp, int) and exp >= 0:
            if exp == 0:
                return self.__class__(1)
            exp = bin(exp)[3:]
            res = self
            for i in exp:
                res = res.square()
                if i == '1':
                    res *= self
            return res
        else:
            return NotImplemented

class _int(object):
    def if_else(self, a, b):
        return self * (a - b) + b

    def cond_swap(self, a, b):
        prod = self * (a - b)
        return a - prod, b + prod

class _gf2n(object):
    def if_else(self, a, b):
        return b ^ self * self.hard_conv(a ^ b)

    def cond_swap(self, a, b, t=None):
        prod = self * self.hard_conv(a ^ b)
        res = a ^ prod, b ^ prod
        if t is None:
            return res
        else:
            return tuple(t.conv(r) for r in res)


class _register(Tape.Register, _number):
    @vectorized_classmethod
    def conv(cls, val):
        if isinstance(val, MemValue):
            val = val.read()
        if isinstance(val, cls):
            return val
        elif not isinstance(val, _register):
            try:
                return type(val)(cls.conv(v) for v in val)
            except TypeError:
                pass
        return cls(val)

    @vectorized_classmethod
    @read_mem_value
    def hard_conv(cls, val):
        if type(val) == cls:
            return val
        elif not isinstance(val, _register):
            try:
                return val.hard_conv_me(cls)
            except AttributeError:
                try:
                    return type(val)(cls.hard_conv(v) for v in val)
                except TypeError:
                    pass
        return cls(val)

    @vectorized_classmethod
    @set_instruction_type
    def _load_mem(cls, address, direct_inst, indirect_inst):
        res = cls()
        if isinstance(address, _register):
            indirect_inst(res, regint.conv(address))
        else:
            direct_inst(res, address)
        return res

    @set_instruction_type
    @vectorize
    def _store_in_mem(self, address, direct_inst, indirect_inst):
        if isinstance(address, _register):
            indirect_inst(self, regint.conv(address))
        else:
            direct_inst(self, address)

    @classmethod
    def prep_res(cls, other):
        return cls()

    def __init__(self, reg_type, val, size):
        super(_register, self).__init__(reg_type, program.curr_tape, size=size)
        if isinstance(val, (int, long)):
            self.load_int(val)
        elif val is not None:
            self.load_other(val)

    def sizeof(self):
        return self.size


class _clear(_register):
    __slots__ = []

    @vectorized_classmethod
    @set_instruction_type
    def protect_memory(cls, start, end):
        program.curr_tape.start_new_basicblock(name='protect-memory')
        protectmemc(regint(start), regint(end))

    @set_instruction_type
    @vectorize
    def load_other(self, val):
        if isinstance(val, type(self)):
            movc(self, val)
        else:
            self.convert_from(val)

    @vectorize
    @read_mem_value
    def convert_from(self, val):
        if not isinstance(val, regint):
            val = regint(val)
        convint(self, val)

    @set_instruction_type
    @vectorize
    def print_reg(self, comment=''):
        print_reg(self, comment)

    @set_instruction_type
    @vectorize
    def print_reg_plain(self):
        print_reg_plain(self)

    @set_instruction_type
    @vectorize
    def raw_output(self):
        raw_output(self)

    @set_instruction_type
    @read_mem_value
    @vectorize
    def clear_op(self, other, c_inst, ci_inst, reverse=False):
        cls = self.__class__
        res = self.prep_res(other)
        if isinstance(other, cls):
            c_inst(res, self, other)
        elif isinstance(other, (int, long)):
            if self.in_immediate_range(other):
                ci_inst(res, self, other)
            else:
                if reverse:
                    c_inst(res, cls(other), self)
                else:
                    c_inst(res, self, cls(other))
        else:
            return NotImplemented
        return res

    @set_instruction_type
    @read_mem_value
    @vectorize
    def coerce_op(self, other, inst, reverse=False):
        cls = self.__class__
        res = cls()
        if isinstance(other, (int, long)):
            other = cls(other)
        elif not isinstance(other, cls):
            return NotImplemented
        if reverse:
            inst(res, other, self)
        else:
            inst(res, self, other)
        return res

    def add(self, other):
        return self.clear_op(other, addc, addci)

    def mul(self, other):
        return self.clear_op(other, mulc, mulci)

    def __sub__(self, other):
        return self.clear_op(other, subc, subci)

    def __rsub__(self, other):
        return self.clear_op(other, subc, subcfi, True)

    def __div__(self, other):
        return self.clear_op(other, divc, divci)

    def __rdiv__(self, other):
        return self.coerce_op(other, divc, True)

    def __eq__(self, other):
        if isinstance(other, (_clear,int,long)):
            return regint(self) == other
        else:
            return NotImplemented

    def __ne__(self, other):
        return 1 - (self == other)

    def __and__(self, other):
        return self.clear_op(other, andc, andci)

    def __xor__(self, other):
        return self.clear_op(other, xorc, xorci)

    def __or__(self, other):
        return self.clear_op(other, orc, orci)

    __rand__ = __and__
    __rxor__ = __xor__
    __ror__ = __or__


class cint(_clear, _int):
    " Clear mod p integer type. """
    __slots__ = []
    instruction_type = 'modp'
    reg_type = 'c'

    @vectorized_classmethod
    def load_mem(cls, address):
        return cls._load_mem(address, ldmc, ldmci)

    def store_in_mem(self, address):
        self._store_in_mem(address, stmc, stmci)

    @staticmethod
    def in_immediate_range(value):
        return value < 2**31 and value >= -2**31

    def __init__(self, val=None, size=None):
        super(cint, self).__init__('c', val=val, size=size)

    @vectorize
    def load_int(self, val):
        if val:
            # +1 for sign
            program.curr_tape.require_bit_length(1 + int(math.ceil(math.log(abs(val)))))
        if self.in_immediate_range(val):
            ldi(self, val)
        else:
            max = 2**31 - 1
            sign = abs(val) / val
            val = abs(val)
            chunks = []
            while val:
                mod = val % max
                val = (val - mod) / max
                chunks.append(mod)
            sum = cint(sign * chunks.pop())
            for i,chunk in enumerate(reversed(chunks)):
                sum *= max
                if i == len(chunks) - 1:
                    addci(self, sum, sign * chunk)
                elif chunk:
                    sum += sign * chunk

    def __mod__(self, other):
        return self.clear_op(other, modc, modci)

    def __rmod__(self, other):
        return self.coerce_op(other, modc, True)

    def __lt__(self, other):
        if isinstance(other, (type(self),int,long)):
            return regint(self) < other
        else:
            return NotImplemented

    def __gt__(self, other):
        if isinstance(other, (type(self),int,long)):
            return regint(self) > other
        else:
            return NotImplemented

    def __le__(self, other):
        return 1 - (self > other)

    def __ge__(self, other):
        return 1 - (self < other)

    def __lshift__(self, other):
        return self.clear_op(other, shlc, shlci)

    def __rshift__(self, other):
        return self.clear_op(other, shrc, shrci)

    def __neg__(self):
        return 0 - self

    @vectorize
    def __invert__(self):
        res = cint()
        notc(res, self, program.bit_length)
        return res

    def __rpow__(self, base):
        if base == 2:
            return 1 << self
        else:
            return NotImplemented

    @vectorize
    def __rlshift__(self, other):
        return cint(other) << self

    @vectorize
    def __rrshift__(self, other):
        return cint(other) >> self

    @read_mem_value
    def mod2m(self, other, bit_length=None, signed=None):
        return self % 2**other

    @read_mem_value
    def right_shift(self, other, bit_length=None):
        return self >> other

    @read_mem_value
    def greater_than(self, other, bit_length=None):
        return self > other

    def pow2(self, bit_length=None):
        return 2**self

    def bit_decompose(self, bit_length=None):
        if bit_length == 0:
            return []
        bit_length = bit_length or program.bit_length
        return floatingpoint.bits(self, bit_length)

    def legendre(self):
        res = cint()
        legendrec(res, self)
        return res


class cgf2n(_clear, _gf2n):
    __slots__ = []
    instruction_type = 'gf2n'
    reg_type = 'cg'

    @classmethod
    def bit_compose(cls, bits, step=None):
        size = bits[0].size
        res = cls(size=size)
        vgbitcom(size, res, step or 1, *bits)
        return res

    @vectorized_classmethod
    def load_mem(cls, address):
        return cls._load_mem(address, gldmc, gldmci)

    def store_in_mem(self, address):
        self._store_in_mem(address, gstmc, gstmci)

    @staticmethod
    def in_immediate_range(value):
        return value < 2**32 and value >= 0

    def __init__(self, val=None, size=None):
        super(cgf2n, self).__init__('cg', val=val, size=size)

    @vectorize
    def load_int(self, val):
        if val < 0:
            raise CompilerError('Negative GF2n immediate')
        if self.in_immediate_range(val):
            gldi(self, val)
        else:
            chunks = []
            while val:
                mod = val % 2**32
                val >>= 32
                chunks.append(mod)
            sum = cgf2n(chunks.pop())
            for i,chunk in enumerate(reversed(chunks)):
                sum <<= 32
                if i == len(chunks) - 1:
                    gaddci(self, sum, chunk)
                elif chunk:
                    sum += chunk

    def __mul__(self, other):
        return super(cgf2n, self).__mul__(other)

    def __neg__(self):
        return self

    @vectorize
    def __invert__(self):
        res = cgf2n()
        gnotc(res, self)
        return res

    @vectorize
    def __lshift__(self, other):
        if isinstance(other, int):
            res = cgf2n()
            gshlci(res, self, other)
            return res
        else:
            return NotImplemented

    @vectorize
    def __rshift__(self, other):
        if isinstance(other, int):
            res = cgf2n()
            gshrci(res, self, other)
            return res
        else:
            return NotImplemented

    @vectorize
    def bit_decompose(self, bit_length=None, step=None):
        bit_length = bit_length or program.galois_length
        step = step or 1
        res = [type(self)() for _ in range(bit_length / step)]
        gbitdec(self, step, *res)
        return res

class regint(_register, _int):
    __slots__ = []
    reg_type = 'ci'
    instruction_type = 'modp'

    @classmethod
    def protect_memory(cls, start, end):
        program.curr_tape.start_new_basicblock(name='protect-memory')
        protectmemint(regint(start), regint(end))

    @vectorized_classmethod
    def load_mem(cls, address):
        return cls._load_mem(address, ldmint, ldminti)

    def store_in_mem(self, address):
        self._store_in_mem(address, stmint, stminti)

    @vectorized_classmethod
    def pop(cls):
        res = cls()
        popint(res)
        return res

    @vectorized_classmethod
    def get_random(cls, bit_length):
        if isinstance(bit_length, int):
            bit_length = regint(bit_length)
        res = cls()
        rand(res, bit_length)
        return res

    @vectorized_classmethod
    def read_from_socket(cls):
        res = cls()
        readsocketc(res,0)
        return res

    @vectorize
    def write_to_socket(self):
        writesocketc(self,0)

    @vectorize_init
    def __init__(self, val=None, size=None):
        super(regint, self).__init__(self.reg_type, val=val, size=size)

    def load_int(self, val):
        if cint.in_immediate_range(val):
            ldint(self, val)
        else:
            lower = val % 2**32
            upper = val >> 32
            if lower >= 2**31:
                lower -= 2**32
                upper += 1
            addint(self, regint(upper) * regint(2**16)**2, regint(lower))

    @read_mem_value
    def load_other(self, val):
        if isinstance(val, cint):
            convmodp(self, val)
        elif isinstance(val, cgf2n):
            gconvgf2n(self, val)
        elif isinstance(val, regint):
            addint(self, val, regint(0))
        else:
            raise CompilerError("Cannot convert '%s' to integer" % type(val))

    @vectorize
    @read_mem_value
    def int_op(self, other, inst, reverse=False):
        if isinstance(other, _secret):
            return NotImplemented
        elif not isinstance(other, type(self)):
            other = type(self)(other)
        res = regint()
        if reverse:
            inst(res, other, self)
        else:
            inst(res, self, other)
        return res

    def add(self, other):
        return self.int_op(other, addint)

    def __sub__(self, other):
        return self.int_op(other, subint)

    def __rsub__(self, other):
        return self.int_op(other, subint, True)

    def mul(self, other):
        return self.int_op(other, mulint)

    def __neg__(self):
        return 0 - self

    def __div__(self, other):
        return self.int_op(other, divint)

    def __rdiv__(self, other):
        return self.int_op(other, divint, True)

    def __mod__(self, other):
        return cint(self) % other

    def __rmod__(self, other):
        return other % cint(self)

    def __rpow__(self, other):
        return other**cint(self)

    def __eq__(self, other):
        return self.int_op(other, eqc)

    def __ne__(self, other):
        return 1 - (self == other)

    def __lt__(self, other):
        return self.int_op(other, ltc)

    def __gt__(self, other):
        return self.int_op(other, gtc)

    def __le__(self, other):
        return 1 - (self > other)

    def __ge__(self, other):
        return 1 - (self < other)

    def __lshift__(self, other):
        return regint(cint(self) << other)

    def __rshift__(self, other):
        return regint(cint(self) >> other)

    def __rlshift__(self, other):
        return regint(other << cint(self))

    def __rrshift__(self, other):
        return regint(other >> cint(self))

    def __and__(self, other):
        return regint(other & cint(self))

    def __or__(self, other):
        return regint(other | cint(self))

    def __xor__(self, other):
        return regint(other ^ cint(self))

    __rand__ = __and__
    __ror__ = __or__
    __rxor__ = __xor__

    def mod2m(self, *args, **kwargs):
        return cint(self).mod2m(*args, **kwargs)


class _secret(_register):
    __slots__ = []

    @vectorized_classmethod
    @set_instruction_type
    def protect_memory(cls, start, end):
        program.curr_tape.start_new_basicblock(name='protect-memory')
        protectmems(regint(start), regint(end))

    @vectorized_classmethod
    @set_instruction_type
    def get_input_from(cls, player):
        res = cls()
        asm_input(res, player)
        return res

    @vectorized_classmethod
    @set_instruction_type
    def get_random_triple(cls):
        res = (cls(), cls(), cls())
        triple(*res)
        return res

    @vectorized_classmethod
    @set_instruction_type
    def get_random_bit(cls):
        res = cls()
        bit(res)
        return res

    @vectorized_classmethod
    @set_instruction_type
    def get_random_square(cls):
        res = (cls(), cls())
        square(*res)
        return res

    @vectorized_classmethod
    @set_instruction_type
    def get_random_inverse(cls):
        res = (cls(), cls())
        inverse(*res)
        return res

    @vectorized_classmethod
    @set_instruction_type
    def get_random_input_mask_for(cls, player):
        res = cls()
        inputmask(res, player)
        return res

    def __init__(self, reg_type, val=None, size=None):
        if isinstance(val, self.clear_type):
            size = val.size
        super(_secret, self).__init__(reg_type, val=val, size=size)

    @set_instruction_type
    @vectorize
    def load_int(self, val):
        if self.clear_type.in_immediate_range(val):
            ldsi(self, val)
        else:
            self.load_clear(self.clear_type(val))

    @vectorize
    def load_clear(self, val):
        addm(self, self.__class__(0), val)

    @set_instruction_type
    @read_mem_value
    @vectorize
    def load_other(self, val):
        if isinstance(val, self.clear_type):
            self.load_clear(val)
        elif isinstance(val, type(self)):
            movs(self, val)
        else:
            self.load_clear(self.clear_type(val))

    @set_instruction_type
    @read_mem_value
    @vectorize
    def secret_op(self, other, s_inst, m_inst, si_inst, reverse=False):
        cls = self.__class__
        res = self.prep_res(other)
        if isinstance(other, regint):
            other = res.clear_type(other)
        if isinstance(other, cls):
            s_inst(res, self, other)
        elif isinstance(other, res.clear_type):
            if reverse:
                m_inst(res, other, self)
            else:
                m_inst(res, self, other)
        elif isinstance(other, (int, long)):
            if self.clear_type.in_immediate_range(other):
                si_inst(res, self, other)
            else:
                if reverse:
                    m_inst(res, res.clear_type(other), self)
                else:
                    m_inst(res, self, res.clear_type(other))
        else:
            return NotImplemented
        return res

    def add(self, other):
        return self.secret_op(other, adds, addm, addsi)

    def mul(self, other):
        return self.secret_op(other, muls, mulm, mulsi)

    def __sub__(self, other):
        return self.secret_op(other, subs, subml, subsi)

    def __rsub__(self, other):
        return self.secret_op(other, subs, submr, subsfi, True)

    @vectorize
    def __div__(self, other):
        return self * (self.clear_type(1) / other)

    @vectorize
    def __rdiv__(self, other):
        a,b = self.get_random_inverse()
        return other * a / (a * self).reveal()

    @set_instruction_type
    @vectorize
    def square(self):
        res = self.__class__()
        sqrs(res, self)
        return res

    @set_instruction_type
    @vectorize
    def reveal(self):
        res = self.clear_type()
        asm_open(res, self)
        return res

    @set_instruction_type
    def reveal_to(self, player):
        masked = self.__class__()
        startprivateoutput(masked, self, player)
        stopprivateoutput(masked.reveal(), player)


class sint(_secret, _int):
    " Shared mod p integer type. """
    __slots__ = []
    instruction_type = 'modp'
    clear_type = cint
    reg_type = 's'

    @vectorized_classmethod
    def get_random_int(cls, bits):
        res = sint()
        comparison.PRandInt(res, bits)
        return res

    @classmethod
    def get_raw_input_from(cls, player):
        res = cls()
        startinput(player, 1)
        stopinput(player, res)
        return res

    @vectorized_classmethod
    def read_from_socket(cls):
        res = cls()
        readsockets(res,0)
        return res

    @vectorize
    def write_to_socket(self):
        writesockets(self,0)

    @vectorized_classmethod
    def load_mem(cls, address):
        return cls._load_mem(address, ldms, ldmsi)

    def store_in_mem(self, address):
        self._store_in_mem(address, stms, stmsi)

    def __init__(self, val=None, size=None):
        super(sint, self).__init__('s', val=val, size=size)

    @vectorize
    def __neg__(self):
        return 0 - self

    @read_mem_value
    @vectorize
    def __lt__(self, other, bit_length=None, security=None):
        res = sint()
        comparison.LTZ(res, self - other, bit_length or program.bit_length + 1,
                       security or program.security)
        return res

    @read_mem_value
    @vectorize
    def __gt__(self, other, bit_length=None, security=None):
        res = sint()
        comparison.LTZ(res, other - self, bit_length or program.bit_length + 1,
                       security or program.security)
        return res

    def __le__(self, other, bit_length=None, security=None):
        return 1 - self.greater_than(other, bit_length, security)

    def __ge__(self, other, bit_length=None, security=None):
        return 1 - self.less_than(other, bit_length, security)

    @read_mem_value
    @vectorize
    def __eq__(self, other, bit_length=None, security=None):
        return floatingpoint.EQZ(self - other, bit_length or program.bit_length,
                                 security or program.security)

    def __ne__(self, other, bit_length=None, security=None):
        return 1 - self.equal(other, bit_length, security)

    less_than = __lt__
    greater_than = __gt__
    less_equal = __le__
    greater_equal = __ge__
    equal = __eq__
    not_equal = __ne__

    @vectorize
    def __mod__(self, modulus):
        if isinstance(modulus, (int, long)):
            l = math.log(modulus, 2)
            if 2**int(round(l)) == modulus:
                return self.mod2m(int(l))
        raise NotImplementedError('Modulo only implemented for powers of two.')

    @read_mem_value
    def mod2m(self, m, bit_length=None, security=None, signed=True):
        bit_length = bit_length or program.bit_length
        security = security or program.security
        if isinstance(m, int):
            if m == 0:
                return 0
            if m >= bit_length:
                return self
            res = sint()
            if m == 1:
                comparison.Mod2(res, self, bit_length, security, signed)
            else:
                comparison.Mod2m(res, self, bit_length, m, security, signed)
        else:
            res, pow2 = floatingpoint.Trunc(self, bit_length, m, security, True)
        return res

    @vectorize
    def __rpow__(self, base):
        if base == 2:
            return self.pow2()
        else:
            return NotImplemented

    def pow2(self, bit_length=None, security=None):
        return floatingpoint.Pow2(self, bit_length or program.bit_length, \
                                      security or program.security)

    def __lshift__(self, other):
        return self * 2**other

    @vectorize
    @read_mem_value
    def __rshift__(self, other, bit_length=None, security=None):
        bit_length = bit_length or program.bit_length
        security = security or program.security
        if isinstance(other, int):
            if other == 0:
                return self
            res = sint()
            comparison.Trunc(res, self, bit_length, other, security, True)
            return res
        elif isinstance(other, sint):
            return floatingpoint.Trunc(self, bit_length, other, security)
        else:
            return floatingpoint.Trunc(self, bit_length, sint(other), security)

    right_shift = __rshift__

    def __rlshift__(self, other):
        return other * 2**self

    @vectorize
    def __rrshift__(self, other):
        return floatingpoint.Trunc(other, program.bit_length, self, program.security)

    def bit_decompose(self, bit_length=None, security=None):
        if bit_length == 0:
            return []
        bit_length = bit_length or program.bit_length
        security = security or program.security
        return floatingpoint.BitDec(self, bit_length, bit_length, security)

class sgf2n(_secret, _gf2n):
    __slots__ = []
    instruction_type = 'gf2n'
    clear_type = cgf2n
    reg_type = 'sg'

    @classmethod
    def get_raw_input_from(cls, player):
        res = cls()
        gstartinput(player, 1)
        gstopinput(player, res)
        return res

    def add(self, other):
        if isinstance(other, sgf2nint):
            return NotImplemented
        else:
            return super(sgf2n, self).add(other)

    def mul(self, other):
        if isinstance(other, (sgf2nint)):
            return NotImplemented
        else:
            return super(sgf2n, self).mul(other)

    @vectorized_classmethod
    def load_mem(cls, address):
        return cls._load_mem(address, gldms, gldmsi)

    def store_in_mem(self, address):
        self._store_in_mem(address, gstms, gstmsi)

    def __init__(self, val=None, size=None):
        super(sgf2n, self).__init__('sg', val=val, size=size)

    def __neg__(self):
        return self

    @vectorize
    def __invert__(self):
        return self ^ cgf2n(2**program.galois_length - 1)

    def __xor__(self, other):
        if other is 0 or other is 0L:
            return self
        else:
            return super(sgf2n, self).add(other)

    __rxor__ = __xor__

    @vectorize
    def __and__(self, other):
        if isinstance(other, (int, long)):
            other_bits = [(other >> i) & 1 \
                              for i in range(program.galois_length)]
        else:
            other_bits = other.bit_decompose()
        self_bits = self.bit_decompose()
        return sum((x * y) << i \
                       for i,(x,y) in enumerate(zip(self_bits, other_bits)))

    __rand__ = __and__

    @vectorize
    def __lshift__(self, other):
        return self * cgf2n(1 << other)

    @vectorize
    def right_shift(self, other, bit_length=None):
        bits = self.bit_decompose(bit_length)
        return sum(b << i for i,b in enumerate(bits[other:]))

    def equal(self, other, bit_length=None, expand=1):
        bits = [1 - bit for bit in (self - other).bit_decompose(bit_length)][::expand]
        while len(bits) > 1:
            bits.insert(0, bits.pop() * bits.pop())
        return bits[0]

    def not_equal(self, other, bit_length=None):
        return 1 - self.equal(other, bit_length)

    __eq__ = equal
    __ne__ = not_equal

    @vectorize
    def bit_decompose(self, bit_length=None, step=1):
        if bit_length == 0:
            return []
        bit_length = bit_length or program.galois_length
        random_bits = [self.get_random_bit() \
                           for i in range(0, bit_length, step)]
        one = cgf2n(1)
        masked = sum([b * (one << (i * step)) for i,b in enumerate(random_bits)], self).reveal()
        masked_bits = masked.bit_decompose(bit_length)
        return [m + r for m,r in zip(masked_bits, random_bits)]

    @vectorize
    def bit_decompose_embedding(self):

        random_bits = [self.get_random_bit() \
                           for i in range(8)]
        one = cgf2n(1)
        wanted_positions = [0, 5, 10, 15, 20, 25, 30, 35]
        masked = sum([b * (one << wanted_positions[i]) for i,b in enumerate(random_bits)], self).reveal()
        return [self.clear_type((masked >> wanted_positions[i]) & one) + r for i,r in enumerate(random_bits)]

sint.basic_type = sint
sgf2n.basic_type = sgf2n


class sgf2nint(sgf2n):
    bits = None

    @classmethod
    def compose(cls, bits):
        bits = list(bits)
        if len(bits) > cls.n_bits:
            raise CompilerError('Too many bits')
        res = cls()
        res.bits = bits + [0] * (cls.n_bits - len(bits))
        gmovs(res, sum(b << i for i,b in enumerate(bits)))
        return res

    @staticmethod
    def bit_adder(a, b):
        a, b = list(a), list(b)
        a += [0] * (len(b) - len(a))
        b += [0] * (len(a) - len(b))
        lower = []
        for (ai,bi) in zip(a,b):
            if ai is 0 or bi is 0:
                lower.append(ai + bi)
                a.pop(0)
                b.pop(0)
            else:
                break
        d = [(ai + bi, ai * bi) for (ai,bi) in zip(a,b)]
        carry = lambda y,x,*args: \
            (x[0] * y[0], 1 - (1 - x[1]) * (1 - x[0] * y[1]))
        if d:
            carries = (0,) + zip(*floatingpoint.PreOpL(carry, d))[1]
        else:
            carries = []
        return lower + [ai + bi + carry for (ai,bi,carry) in zip(a,b,carries)]

    @staticmethod
    def full_adder(a, b, carry):
        s = a + b
        return s + carry, util.or_op(a * b, s * carry)

    @staticmethod
    def half_adder(a, b):
        return a + b, a * b

    @staticmethod
    def bit_comparator(a, b):
        op = lambda y,x,*args: (util.if_else(x[1], x[0], y[0]), \
                                    util.if_else(x[1], 1, y[1]))
        return floatingpoint.KOpL(op, [(bi, ai + bi) for (ai,bi) in zip(a,b)])        

    @staticmethod
    def get_highest_different_bits(a, b, index):
        diff = [ai + bi for (ai,bi) in reversed(zip(a,b))]
        preor = floatingpoint.PreOR(diff, raw=True)
        highest_diff = [x - y for (x,y) in reversed(zip(preor, [0] + preor))]
        raw = sum(map(operator.mul, highest_diff, (a,b)[index]))
        return raw.bit_decompose()[0]

    def load_int(self, other):
        if -2**(self.n_bits-1) <= other < 2**(self.n_bits-1):
            sgf2n.load_int(self, other + 2**self.n_bits if other < 0 else other)
        else:
            raise CompilerError('Invalid signed %d-bit integer: %d' % \
                                    (self.n_bits, other))

    def load_other(self, other):
        if isinstance(other, sgf2nint):
            gmovs(self, self.compose(other.bit_decompose(self.n_bits)))
        elif isinstance(other, sgf2n):
            gmovs(self, other)
        else:
            gaddm(self, sgf2n(0), cgf2n(other))

    def add(self, other):
        if type(other) == sgf2n:
            raise CompilerError('Unclear addition')
        a = self.bit_decompose()
        b = util.bit_decompose(other, self.n_bits)
        return self.compose(self.bit_adder(a, b))

    def mul(self, other):
        if type(other) == sgf2n:
            raise CompilerError('Unclear multiplication')
        self_bits = self.bit_decompose()
        if isinstance(other, (int, long)):
            other_bits = util.bit_decompose(other, self.n_bits)
            bit_matrix = [[x * y for y in self_bits] for x in other_bits]
        else:
            other = sgf2n(other)
            products = [x * other for x in self_bits]
            bit_matrix = [util.bit_decompose(x, self.n_bits) for x in products]
        columns = [filter(None, (bit_matrix[j][i-j] \
                                     for j in range(min(len(bit_matrix), i + 1)))) \
                       for i in range(len(bit_matrix[0]))]
        # Wallace tree
        while max(len(c) for c in columns) > 2:
            new_columns = [[] for i in range(len(columns) + 1)]
            for i,col in enumerate(columns):
                while len(col) > 2:
                    s, carry = self.full_adder(*(col.pop() for i in range(3)))
                    new_columns[i].append(s)
                    new_columns[i+1].append(carry)
                if len(col) == 2:
                    s, carry = self.half_adder(*(col.pop() for i in range(2)))
                    new_columns[i].append(s)
                    new_columns[i+1].append(carry)
                else:
                    new_columns[i].extend(col)
            columns = new_columns[:-1]
        for col in columns:
            col.extend([0] * (2 - len(col)))
        return self.compose(self.bit_adder(*zip(*columns)))

    def __sub__(self, other):
        if type(other) == sgf2n:
            raise CompilerError('Unclear subtraction')
        a = self.bit_decompose()
        b = util.bit_decompose(other, self.n_bits)
        d = [(1 + ai + bi, (1 - ai) * bi) for (ai,bi) in zip(a,b)]
        borrow = lambda y,x,*args: \
            (x[0] * y[0], 1 - (1 - x[1]) * (1 - x[0] * y[1]))
        borrows = (0,) + zip(*floatingpoint.PreOpL(borrow, d))[1]
        return self.compose(ai + bi + borrow \
                                for (ai,bi,borrow) in zip(a,b,borrows))

    def __rsub__(self, other):
        raise NotImplementedError()

    def __div__(self, other):
        raise NotImplementedError()

    def __rdiv__(self, other):
        raise NotImplementedError()

    def __lshift__(self, other):
        return self.compose(([0] * other + self.bit_decompose())[:self.n_bits])

    def __rshift__(self, other):
        return self.compose(self.bit_decompose()[other:])

    def bit_decompose(self, n_bits=None, *args):
        if self.bits is None:
            self.bits = sgf2n(self).bit_decompose(self.n_bits)
        if n_bits is None:
            return self.bits[:]
        else:
            return self.bits[:n_bits] + [self.fill_bit()] * (n_bits - self.n_bits)

    def fill_bit(self):
        return self.bits[-1]

    @staticmethod
    def prep_comparison(a, b):
        a[-1], b[-1] = b[-1], a[-1]
    
    def comparison(self, other, const_rounds=False, index=None):
        a = self.bit_decompose()
        b = util.bit_decompose(other, self.n_bits)
        self.prep_comparison(a, b)
        if const_rounds:
            return self.get_highest_different_bits(a, b, index)
        else:
            return self.bit_comparator(a, b)

    def __lt__(self, other):
        if program.options.comparison == 'log':
            x, not_equal = self.comparison(other)
            return util.if_else(not_equal, x, 0)
        else:
            return self.comparison(other, True, 1)

    def __le__(self, other):
        if program.options.comparison == 'log':
            x, not_equal = self.comparison(other)
            return util.if_else(not_equal, x, 1)
        else:
            return 1 - self.comparison(other, True, 0)

    def __ge__(self, other):
        return 1 - (self < other)

    def __gt__(self, other):
        return 1 - (self <= other)

    def __neg__(self):
        return 1 + self.compose(1 ^ b for b in self.bit_decompose())

class sgf2nuint(sgf2nint):
    def load_int(self, other):
        if 0 <= other < 2**self.n_bits:
            sgf2n.load_int(self, other)
        else:
            raise CompilerError('Invalid unsigned %d-bit integer: %d' % \
                                    (self.n_bits, other))

    def fill_bit(self):
        return 0

    @staticmethod
    def prep_comparison(a, b):
        pass

class sgf2nuint32(sgf2nuint):
    n_bits = 32

class sgf2nint32(sgf2nint):
    n_bits = 32

def get_sgf2nint(n):
    class sgf2nint_spec(sgf2nint):
        n_bits = n
    #sgf2nint_spec.__name__ = 'sgf2unint' + str(n)
    return sgf2nint_spec

def get_sgf2nuint(n):
    class sgf2nuint_spec(sgf2nint):
        n_bits = n
    #sgf2nuint_spec.__name__ = 'sgf2nuint' + str(n)
    return sgf2nuint_spec

class sgf2nfloat(sgf2n):
    @classmethod
    def set_precision(cls, vlen, plen):
        cls.vlen = vlen
        cls.plen = plen
        class v_type(sgf2nuint):
            n_bits = 2 * vlen + 1
        class p_type(sgf2nint):
            n_bits = plen
        class pdiff_type(sgf2nuint):
            n_bits = plen
        cls.v_type = v_type
        cls.p_type = p_type
        cls.pdiff_type = pdiff_type

    def __init__(self, val, p=None, z=None, s=None):
        super(sgf2nfloat, self).__init__()
        if p is None and type(val) == sgf2n:
            bits = val.bit_decompose(self.vlen + self.plen + 1)
            self.v = self.v_type.compose(bits[:self.vlen])
            self.p = self.p_type.compose(bits[self.vlen:-1])
            self.s = bits[-1]
            self.z = util.tree_reduce(operator.mul, (1 - b for b in self.v.bits))
        else:
            if p is None:
                v, p, z, s = sfloat.convert_float(val, self.vlen, self.plen)
                # correct sfloat
                p += self.vlen - 1
                v_bits = util.bit_decompose(v, self.vlen)
                p_bits = util.bit_decompose(p, self.plen)
                self.v = self.v_type.compose(v_bits)
                self.p = self.p_type.compose(p_bits)
                self.z = z
                self.s = s
            else:
                self.v, self.p, self.z, self.s = val, p, z, s
                v_bits = val.bit_decompose()[:self.vlen]
                p_bits = p.bit_decompose()[:self.plen]
            gmovs(self, util.bit_compose(v_bits + p_bits + [self.s]))

    def add(self, other):
        a = self.p < other.p
        b = self.p == other.p
        c = self.v < other.v
        other_dominates = (b.if_else(c, a))
        pmax, pmin = a.cond_swap(self.p, other.p, self.p_type)
        vmax, vmin = other_dominates.cond_swap(self.v, other.v, self.v_type)
        s3 = self.s ^ other.s
        pdiff = self.pdiff_type(pmax - pmin)
        d = self.vlen < pdiff
        pow_delta = util.pow2(d.if_else(0, pdiff).bit_decompose(util.log2(self.vlen)))
        v3 = vmax
        v4 = self.v_type(sgf2n(vmax) * pow_delta) + self.v_type(s3.if_else(-vmin, vmin))
        v = self.v_type(sgf2n(d.if_else(v3, v4) << self.vlen) / pow_delta)
        v >>= self.vlen - 1
        h = floatingpoint.PreOR(v.bits[self.vlen+1::-1])
        tmp = sum(util.if_else(b, 0, 1 << i) for i,b in enumerate(h))
        pow_p0 = 1 + self.v_type(tmp)
        v = (v * pow_p0) >> 2
        p = pmax - sum(self.p_type.compose([1 - b]) for b in h) + 1
        v = self.z.if_else(other.v, other.z.if_else(self.v, v))
        z = v == 0
        p = z.if_else(0, self.z.if_else(other.p, other.z.if_else(self.p, p)))
        s = other_dominates.if_else(other.s, self.s)
        s = self.z.if_else(other.s, other.z.if_else(self.s, s))
        return sgf2nfloat(v, p, z, s)

    def mul(self, other):
        v = (self.v * other.v) >> (self.vlen - 1)
        b = v.bits[self.vlen]
        v = b.if_else(v >> 1, v)
        p = self.p + other.p + self.p_type.compose([b])
        s = self.s + other.s
        z = util.or_op(self.z, other.z)
        return sgf2nfloat(v, p, z, s)

sgf2nfloat.set_precision(24, 8)

def parse_type(other):
    # converts type to cfix/sfix depending on the case
    if isinstance(other, cfix.scalars):
        return cfix(other)
    elif isinstance(other, cint):
        tmp = cfix()
        tmp.load_int(other)
        return tmp
    elif isinstance(other, sint):
        tmp = sfix()
        tmp.load_int(other)
        return tmp
    elif isinstance(other, sfloat):
        tmp = sfix(other)
        return tmp
    else:
        return other

class cfix(_number):
    """ Clear fixed point type. """
    __slots__ = ['value', 'f', 'k', 'size']
    reg_type = 'c'
    scalars = (int, long, float)
    @classmethod
    def set_precision(cls, f, k = None):
        # k is the whole bitlength of fixed point
        # f is the bitlength of decimal part
        cls.f = f
        if k is None:
            cls.k = 2 * f
        else:
            cls.k = k

    @vectorized_classmethod
    def load_mem(cls, address, mem_type=None):
        res = []
        res.append(cint.load_mem(address))
        return cfix(*res)

    @vectorize_init
    def __init__(self, v=None, size=None):
        f = self.f
        k = self.k
        self.size = get_global_vector_size()
        if isinstance(v, cint):
            self.v = cint(v,size=self.size)
        elif isinstance(v, cfix.scalars):
            self.v = cint(int(round(v * (2 ** f))),size=self.size)
        elif isinstance(v, cfix):
            self.v = v.v
        elif isinstance(v, MemValue):
            self.v = v
        elif v == None:
            self.v = 0

    @vectorize
    def load_int(self, v):
        self.v = cint(v) * (2 ** self.f)

    def conv(self):
        return self

    def store_in_mem(self, address):
        self.v.store_in_mem(address)

    def sizeof(self):
        return self.size * 4

    @vectorize
    def add(self, other):
        other = parse_type(other)
        if isinstance(other, cfix):
            return cfix(self.v + other.v)
        elif isinstance(other, sfix):
            return sfix(self.v + other.v)
        else:
            raise CompilerError('Invalid type %s for cfix.__add__' % type(other))

    @vectorize 
    def mul(self, other):
        other = parse_type(other)
        if isinstance(other, cfix):
            sgn = cint(1 - 2 * (self.v * other.v < 0))
            absolute = self.v * other.v * sgn
            val = sgn * (absolute >> self.f)
            return cfix(val)
        elif isinstance(other, sfix):
            res = sfix((self.v * other.v) >> self.f)
            return res
        else:
            raise CompilerError('Invalid type %s for cfix.__mul__' % type(other))
    
    @vectorize
    def __sub__(self, other):
        other = parse_type(other)
        if isinstance(other, cfix):
            return cfix(self.v - other.v)
        elif isinstance(other, sfix):
            return sfix(self.v - other.v)
        else:
            raise NotImplementedError

    @vectorize
    def __neg__(self):
        # cfix type always has .v
        return cfix(-self.v)
    
    def __rsub__(self, other):
        return -self + other

    @vectorize
    def __eq__(self, other):
        other = parse_type(other)
        if isinstance(other, cfix):
            return self.v == other.v
        elif isinstance(other, sfix):
            return other.v.equal(self.v, self.k, other.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __lt__(self, other):
        other = parse_type(other)
        if isinstance(other, cfix):
            return self.v < other.v
        elif isinstance(other, sfix):
            if(self.k != other.k or self.f != other.f):
                raise TypeError('Incompatible fixed point types in comparison')
            return other.v.greater_than(self.v, self.k, other.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __le__(self, other):
        other = parse_type(other)
        if isinstance(other, cfix):
            return self.v <= other.v
        elif isinstance(other, sfix):
            return other.v.greater_equal(self.v, self.k, other.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __gt__(self, other):
        other = parse_type(other)
        if isinstance(other, cfix):
            return self.v > other.v
        elif isinstance(other, sfix):
            return other.v.less_than(self.v, self.k, other.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __ge__(self, other):
        other = parse_type(other)
        if isinstance(other, cfix):
            return self.v >= other.v
        elif isinstance(other, sfix):
            return other.v.less_equal(self.v, self.k, other.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __ne__(self, other):
        other = parse_type(other)
        if isinstance(other, cfix):
            return self.v != other.v
        elif isinstance(other, sfix):
            return other.v.not_equal(self.v, self.k, other.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __div__(self, other):
        other = parse_type(other)
        if isinstance(other, cfix):
            return cfix(library.cint_cint_division(self.v, other.v, self.k, self.f))
        elif isinstance(other, sfix):
            return sfix(library.FPDiv(self.v, other.v, self.k, self.f, other.kappa))
        else:
            raise TypeError('Incompatible fixed point types in division')

class sfix(_number):
    """ Shared fixed point type. """
    __slots__ = ['v', 'f', 'k', 'size']
    reg_type = 's'
    kappa = 40
    @classmethod
    def set_precision(cls, f, k = None):
        cls.f = f
        # default bitlength = 2*precision
        if k is None:
            cls.k = 2 * f
        else:
            cls.k = k

    @vectorized_classmethod
    def load_mem(cls, address, mem_type=None):
        res = []
        res.append(sint.load_mem(address))
        return sfix(*res)

    @vectorize_init
    def __init__(self, _v=None, size=None):
        self.size = get_global_vector_size()
        f = self.f
        k = self.k
        # warning: don't initialize a sfix from a sint, this is only used in internal methods;
        # for external initialization use load_int.
        if isinstance(_v, sint):
            self.v = _v
        elif isinstance(_v, cfix.scalars):
            self.v = sint(int(round(_v * (2 ** f))), size=self.size)
        elif isinstance(_v, sfloat):
            p = (f + _v.p)
            b = (p >= 0)
            a = b*(_v.v << (p)) + (1-b)*(_v.v >> (-p))
            self.v = (1-2*_v.s)*a
        elif isinstance(_v, sfix):
            self.v = _v.v
        elif isinstance(_v, MemValue):
            #this is a memvalue object
            self.v = _v
        elif _v == None:
            self.v = sint(0)
        self.kappa = sfix.kappa 

    @vectorize
    def load_int(self, v):
        self.v = sint(v) << self.f

    def conv(self):
        return self

    def store_in_mem(self, address):
        self.v.store_in_mem(address)

    def sizeof(self):
        return self.size * 4

    @vectorize 
    def add(self, other):
        other = parse_type(other)
        if isinstance(other, (sfix, cfix)):
            return sfix(self.v + other.v)
        elif isinstance(other, cfix.scalars):
            tmp = cfix(other)
            return self + tmp
        else:
            raise CompilerError('Invalid type %s for sfix.__add__' % type(other))

    @vectorize 
    def mul(self, other):
        other = parse_type(other)
        if isinstance(other, sfix):
            val = floatingpoint.TruncPr(self.v * other.v, self.k * 2, self.f, self.kappa)
            return sfix(val)
        elif isinstance(other, cfix):
            res = sfix((self.v * other.v) >> sfix.f)
            return res
        elif isinstance(other, cfix.scalars):
            scalar_fix = cfix(other)
            return self * scalar_fix
        else:
            raise CompilerError('Invalid type %s for sfix.__mul__' % type(other))

    @vectorize 
    def __sub__(self, other):
        other = parse_type(other)
        return self + (-other)

    @vectorize
    def __neg__(self):
        return sfix(-self.v)

    def __rsub__(self, other):
        return -self + other

    @vectorize
    def __eq__(self, other):
        other = parse_type(other)
        if isinstance(other, (cfix, sfix)):
            return self.v.equal(other.v, self.k, self.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __le__(self, other):
        other = parse_type(other)
        if isinstance(other, (cfix, sfix)):
            return self.v.less_equal(other.v, self.k, self.kappa)
        else:
            raise NotImplementedError

    @vectorize 
    def __lt__(self, other):
        other = parse_type(other)
        if isinstance(other, (cfix, sfix)):
            return self.v.less_than(other.v, self.k, self.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __ge__(self, other):
        other = parse_type(other)
        if isinstance(other, (cfix, sfix)):
            return self.v.greater_equal(other.v, self.k, self.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __gt__(self, other):
        other = parse_type(other)
        if isinstance(other, (cfix, sfix)):
            return self.v.greater_than(other.v, self.k, self.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __ne__(self, other):
        other = parse_type(other)
        if isinstance(other, (cfix, sfix)):
            return self.v.not_equal(other.v, self.k, self.kappa)
        else:
            raise NotImplementedError

    @vectorize
    def __div__(self, other):
        other = parse_type(other)
        if isinstance(other, sfix):
            return sfix(library.FPDiv(self.v, other.v, self.k, self.f, self.kappa))
        elif isinstance(other, cfix):
            return sfix(library.sint_cint_division(self.v, other.v, self.k, self.f, self.kappa))
        else:
            raise TypeError('Incompatible fixed point types in division')

    @vectorize
    def compute_reciprocal(self):
        return sfix(library.FPDiv(cint(2) ** self.f, self.v, self.k, self.f, self.kappa, True))

    def reveal(self):
        val = self.v.reveal()
        return cfix(val)

# this is for 20 bit decimal precision
# with 40 bitlength of entire number
# these constants have been chosen for multiplications to fit in 128 bit prime field
# (precision n1) 41 + (precision n2) 41 + (stat_sec) 40 = 82 + 40 = 122 <= 128
# with statistical security of 40

sfix.set_precision(20, 41)
cfix.set_precision(20, 41)

class sfloat(_number):
    """ Shared floating point data type, representing (1 - 2s)*(1 - z)*v*2^p.
        
        v: significand
        p: exponent
        z: zero flag
        s: sign bit
        """
    __slots__ = ['v', 'p', 'z', 's', 'size']
    # single precision
    vlen = 24
    plen = 8
    kappa = 40
    round_nearest = False
    error = 0
    reg_type = 's'

    def conv(self):
        return self

    @vectorized_classmethod
    def load_mem(cls, address):
        res = []
        for i in range(4):
            res.append(sint.load_mem(address + i * get_global_vector_size()))
        return sfloat(*res)


    @classmethod
    def set_error(cls, error):
        cls.error += error - cls.error * error

    @staticmethod
    def convert_float(v, vlen, plen):
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
            vv = v
            v = int(round(abs(v) * 2 ** (-p)))
            if v == 2 ** vlen:
                p += 1
                v /= 2
            z = 0
            if p < -2 ** (plen - 1):
                print 'Warning: %e truncated to zero' % vv
                v, p, z = 0, 0, 1
            if p >= 2 ** (plen - 1):
                raise CompilerError('Cannot convert %s to float ' \
                                        'with %d exponent bits' % (vv, plen))
        return v, p, z, s

    @vectorize_init
    def __init__(self, v, p=None, z=None, s=None, size=None):
        self.size = get_global_vector_size()
        if p is None:
            if isinstance(v, sfloat):
                p = v.p
                z = v.z
                s = v.s
                v = v.v
            elif isinstance(v, sint):
                v, p, z, s = floatingpoint.Int2FL(v, program.bit_length,
                                                  self.vlen, self.kappa)
            elif isinstance(v, sfix):
                f = v.f
                v, p, z, s = floatingpoint.Int2FL(v.v, v.k,
                                                  self.vlen, self.kappa)
                p = p - f
            else:
                v, p, z, s = self.convert_float(v, self.vlen, self.plen)
        if isinstance(v, int):
            if not ((v >= 2**(self.vlen-1) and v < 2**(self.vlen)) or v == 0):
                raise CompilerError('Floating point number malformed: significand')
            self.v = library.load_int_to_secret(v)
        else:
            self.v = v
        if isinstance(p, int):
            if not (p >= -2**(self.plen - 1) and p < 2**(self.plen - 1)):
                raise CompilerError('Floating point number malformed: exponent %d not unsigned %d-bit integer' % (p, self.plen))
            self.p = library.load_int_to_secret(p)
        else:
            self.p = p
        if isinstance(z, int):
            if not (z == 0 or z == 1):
                raise CompilerError('Floating point number malformed: zero bit')
            self.z = sint()
            ldsi(self.z, z)
        else:
            self.z = z
        if isinstance(s, int):
            if not (s == 0 or s == 1):
                raise CompilerError('Floating point number malformed: sign')
            self.s = sint()
            ldsi(self.s, s)
        else:
            self.s = s

    def store_in_mem(self, address):
        for i,x in enumerate((self.v, self.p, self.z, self.s)):
            x.store_in_mem(address + i * get_global_vector_size())

    def sizeof(self):
        return self.size * 4

    @vectorize
    def add(self, other):
        if isinstance(other, sfloat):
            a,c,d,e = [sint() for i in range(4)]
            t = sint()
            t2 = sint()
            v1 = self.v
            v2 = other.v
            p1 = self.p
            p2 = other.p
            s1 = self.s
            s2 = other.s
            z1 = self.z
            z2 = other.z
            a = p1.less_than(p2, self.plen, self.kappa)
            b = floatingpoint.EQZ(p1 - p2, self.plen, self.kappa)
            c = v1.less_than(v2, self.vlen, self.kappa)
            ap1 = a*p1
            ap2 = a*p2
            aneg = 1 - a
            bneg = 1 - b
            cneg = 1 - c
            av1 = a*v1
            av2 = a*v2
            cv1 = c*v1
            cv2 = c*v2
            pmax = ap2 + p1 - ap1
            pmin = p2 - ap2 + ap1
            vmax = bneg*(av2 + v1 - av1) + b*(cv2 + v1 - cv1)
            vmin = bneg*(av1 + v2 - av2) + b*(cv1 + v2 - cv2)
            s3 = s1 + s2 - 2 * s1 * s2
            comparison.LTZ(d, self.vlen + pmin - pmax + sfloat.round_nearest,
                           self.plen, self.kappa)
            pow_delta = floatingpoint.Pow2((1 - d) * (pmax - pmin),
                                           self.vlen + 1 + sfloat.round_nearest,
                                           self.kappa)
            # deviate from paper for more precision
            #v3 = 2 * (vmax - s3) + 1
            v3 = vmax
            v4 = vmax * pow_delta + (1 - 2 * s3) * vmin
            v = (d * v3 + (1 - d) * v4) * two_power(self.vlen + sfloat.round_nearest) \
                * floatingpoint.Inv(pow_delta)
            comparison.Trunc(t, v, 2 * self.vlen + 1 + sfloat.round_nearest,
                             self.vlen - 1, self.kappa, False)
            v = t
            u = floatingpoint.BitDec(v, self.vlen + 2 + sfloat.round_nearest,
                                     self.vlen + 2 + sfloat.round_nearest, self.kappa,
                                     range(1 + sfloat.round_nearest,
                                           self.vlen + 2 + sfloat.round_nearest))
            # using u[0] doesn't seem necessary
            h = floatingpoint.PreOR(u[:sfloat.round_nearest:-1], self.kappa)
            p0 = self.vlen + 1 - sum(h)
            pow_p0 = 1 + sum([two_power(i) * (1 - h[i]) for i in range(len(h))])
            if self.round_nearest:
                t2, overflow = \
                    floatingpoint.TruncRoundNearestAdjustOverflow(pow_p0 * v,
                                                                  self.vlen + 3,
                                                                  self.vlen,
                                                                  self.kappa)
                p0 = p0 - overflow
            else:
                comparison.Trunc(t2, pow_p0 * v, self.vlen + 2, 2, self.kappa, False)
            v = t2
            # deviate for more precision
            #p = pmax - p0 + 1 - d
            p = pmax - p0 + 1
            zz = self.z*other.z
            zprod = 1 - self.z - other.z + zz
            v = zprod*t2 + self.z*v2 + other.z*v1
            z = floatingpoint.EQZ(v, self.vlen, self.kappa)
            p = (zprod*p + self.z*p2 + other.z*p1)*(1 - z)
            s = (1 - b)*(a*other.s + aneg*self.s) + b*(c*other.s + cneg*self.s)
            s = zprod*s + (other.z - zz)*self.s + (self.z - zz)*other.s
            return sfloat(v, p, z, s)
        else:
            return NotImplemented
    
    @vectorize
    def mul(self, other):
        if isinstance(other, sfloat):
            v1 = sint()
            v2 = sint()
            b = sint()
            c2expl = cint()
            comparison.ld2i(c2expl, self.vlen)
            if sfloat.round_nearest:
                v1 = comparison.TruncRoundNearest(self.v*other.v, 2*self.vlen,
                                             self.vlen-1, self.kappa)
            else:
                comparison.Trunc(v1, self.v*other.v, 2*self.vlen, self.vlen-1, self.kappa, False)
            t = v1 - c2expl
            comparison.LTZ(b, t, self.vlen+1, self.kappa)
            comparison.Trunc(v2, b*v1 + v1, self.vlen+1, 1, self.kappa, False)
            z = self.z + other.z - self.z*other.z       # = OR(z1, z2)
            s = self.s + other.s - 2*self.s*other.s     # = XOR(s1,s2)
            p = (self.p + other.p - b + self.vlen)*(1 - z)
            return sfloat(v2, p, z, s)
        else:
            return NotImplemented
    
    def __sub__(self, other):
        return self + -other
    
    def __rsub__(self, other):
        raise NotImplementedError()

    def __div__(self, other):
        v = floatingpoint.SDiv(self.v, other.v + other.z * (2**self.vlen - 1),
                               self.vlen, self.kappa)
        b = v.less_than(two_power(self.vlen-1), self.vlen + 1, self.kappa)
        overflow = v.greater_equal(two_power(self.vlen), self.vlen + 1, self.kappa)
        underflow = v.less_than(two_power(self.vlen-2), self.vlen + 1, self.kappa)
        v = (v + b * v) * (1 - overflow) * (1 - underflow) + \
            overflow * (2**self.vlen - 1) + \
            underflow * (2**(self.vlen-1)) * (1 - self.z)
        p = (1 - self.z) * (self.p - other.p - self.vlen - b + 1)
        z = self.z
        s = self.s + other.s - 2 * self.s * other.s
        sfloat.set_error(other.z)
        return sfloat(v, p, z, s)

    @vectorize
    def __neg__(self):
        return sfloat(self.v, self.p,  self.z, (1 - self.s) * (1 - self.z))
    
    @vectorize
    def __lt__(self, other):
        if isinstance(other, sfloat):
            z1 = self.z
            z2 = other.z
            s1 = self.s
            s2 = other.s
            a = self.p.less_than(other.p, self.plen, self.kappa)
            c = floatingpoint.EQZ(self.p - other.p, self.plen, self.kappa)
            d = ((1 - 2*self.s)*self.v).less_than((1 - 2*other.s)*other.v, self.vlen + 1, self.kappa)
            cd = c*d
            ca = c*a
            b1 = cd + a - ca
            b2 = cd + 1 + ca - c - a
            s12 = self.s*other.s
            z12 = self.z*other.z
            b = (z1 - z12)*(1 - s2) + (z2 - z12)*s1 + (1 + z12 - z1 - z2)*(s1 - s12 + (1 + s12 - s1 - s2)*b1 + s12*b2)
            return b
        else:
            return NotImplemented
    
    def __ge__(self, other):
        return 1 - (self < other)

    @vectorize
    def __eq__(self, other):
        # the sign can be both ways for zeroes
        both_zero = self.z * other.z
        return floatingpoint.EQZ(self.v - other.v, self.vlen, self.kappa) * \
            floatingpoint.EQZ(self.p - other.p, self.plen, self.kappa) * \
            (1 - self.s - other.s + 2 * self.s * other.s) * \
            (1 - both_zero) + both_zero

    def __ne__(self, other):
        return 1 - (self == other)

    def value(self):
        """ Gets actual floating point value, if emulation is enabled. """
        return (1 - 2*self.s.value)*(1 - self.z.value)*self.v.value/float(2**self.p.value)

    def reveal(self):
        return cfloat(self.v.reveal(), self.p.reveal(), self.z.reveal(), self.s.reveal())

class cfloat(object):
    # Helper class used for printing sfloats
    __slots__ = ['v', 'p', 'z', 's']

    def __init__(self, v, p, z, s):
        self.v, self.p, self.z, self.s = [cint.conv(x) for x in (v, p, z, s)]

    def print_float_plain(self):
        print_float_plain(self.v, self.p, self.z, self.s)

_types = {
    'c': cint,
    's': sint,
    'sg': sgf2n,
    'cg': cgf2n,
    'ci': regint,
}


class Array(object):


    def __init__(self, length, value_type, address=None):
        """
        Initializes an Array
        :param length: the length of the array
        :param value_type: the type of the array
        :param address: The address on the tape to store the array
        """
        if value_type in _types:
            value_type = _types[value_type]
        if value_type == sfloat:
            self.internalLength = length * 4
        else:
            self.internalLength = length
        self.address = address
        if address is None:
            self.address = program.malloc(self.internalLength, value_type.reg_type)
        self.value_type = value_type
        self.length = length


    def delete(self):
        if program:
            program.free(self.address, self.value_type.reg_type)

    def __treat_negative_index(self, index):
        # Corrects a negative index
        index += self.length * (index < 0)
        return index

    def __get_address(self, index):
        # returns the internal address of the index
        if isinstance(index, int) and self.internalLength is not None:
            index = self.__treat_negative_index(index)
            if index >= self.length or index < 0:
                raise IndexError('index %s, length %s' % (str(index), str(self.length)))

        # Correct the index for sfloat/sfloatE
        if self.value_type == sfloat:
            index *= 4
        return self.address + index

    def __get_slice(self, index):
        # Returns a meaningful slice
        if index.stop is None and self.internalLength is None:
            raise CompilerError('Cannot slice array of unknown length')
        return self.__treat_negative_index(index.start) or 0, self.__treat_negative_index(
            index.stop) or self.length, index.step or 1

    def __getitem__(self, index):
        """
        Used to access an element of the array, e.g. x = A[2]
        :param index: the index to access. Can be a slice, e.g., A[2:4]
        :return: the element or the subArray
        """
        if isinstance(index, slice):
            start, stop, step = self.__get_slice(index)
            res_length = (stop - start - 1) / step + 1
            res = Array(res_length, self.value_type)

            @library.for_range(res_length)
            def f(i):
                res[i] = self[start + i * step]

            return res
        else:
            return self.value_type.load_mem(self.__get_address(index))

    def __setitem__(self, index, value):
        """
        Used to set an element or a subarray of the array, e.g. A[2] = x
        :param index: the index to modify. Can be a slice, e.g., A[2:4]
        """
        if isinstance(index, slice):
            start, stop, step = self.__get_slice(index)
            source_index = MemValue(0)

            @library.for_range(start, stop, step)
            def f(i):
                self[i] = value[source_index]
                source_index.iadd(1)

            return
        else:
            if type(value) != self.value_type:
                print '\033[93mWarning: setting item in array of different type: ', type(
                    value), 'instead of', self.value_type, '\033[0m'
            self.value_type.conv(value).store_in_mem(self.__get_address(index))

    def __len__(self):
        return self.length

    def __iter__(self):
        for i in range(self.length):
            yield self[i]

    def assign(self, other):
        if isinstance(other, Array):
            def loop(i):
                self[i] = other[i]

            library.range_loop(loop, len(self))
        elif isinstance(other, Tape.Register):
            if len(other) == len(self):
                self[0] = other
            else:
                raise CompilerError('Length mismatch between array and vector')
        else:
            for i, j in enumerate(other):
                self[i] = j
        return self

    def assign_all(self, value):
        """
        Sets all the elements of the array to the given value
        :param value: the value to set to
        """
        tp = type(value)
        mem_value = Array(1, tp)
        mem_value[0] = value
        n_loops = 8 if len(self) > 2 ** 20 else 1

        @library.for_range_multithread(n_loops, 1024, len(self))
        def f(i):
            self[i] = mem_value[0]

        return self


class Matrix(object):
    def __init__(self, rows, columns, value_type):
        self.rows = rows
        self.columns = columns
        if value_type in _types:
            value_type = _types[value_type]
        self.value_type = value_type
        self.address = Array(rows * columns, value_type).address

    def __getitem__(self, index):
        if self.value_type == sfloat:
            index = index * 4
        return Array(self.columns, self.value_type, \
                     self.address + index * self.columns)

    def __len__(self):
        return self.rows

    def assign_all(self, value):
        @library.for_range(len(self))
        def f(i):
            self[i].assign_all(value)
        return self


class SubMultiArray(object):
    def __init__(self, sizes, value_type, address, index):
        self.sizes = sizes
        self.value_type = value_type
        self.address = address + index * reduce(operator.mul, self.sizes)

    def __getitem__(self, index):
        if len(self.sizes) == 2:
            return Array(self.sizes[1], self.value_type, \
                             self.address + index *  self.sizes[0])
        else:
            return SubMultiArray(self.sizes[1:], self.value_type, \
                                     self.address, index)

class MultiArray(object):
    def __init__(self, sizes, value_type):
        self.sizes = sizes
        self.value_type = value_type
        self.array = Array(reduce(operator.mul, sizes), \
                                 value_type)
        if len(sizes) < 2:
            raise CompilerError('Use Array')

    def __getitem__(self, index):
        return SubMultiArray(self.sizes[1:], self.value_type, \
                                 self.array.address, index)

class VectorArray(object):
    def __init__(self, length, value_type, vector_size, address=None):
        self.array = Array(length * vector_size, value_type, address)
        self.vector_size = vector_size
        self.value_type = value_type

    def __getitem__(self, index):
        return self.value_type.load_mem(self.array.address + \
                                        index * self.vector_size,
                                        size=self.vector_size)

    def __setitem__(self, index, value):
        if value.size != self.vector_size:
            raise CompilerError('vector size mismatch')
        value.store_in_mem(self.array.address + index * self.vector_size)

class _mem(_number):
    __add__ = lambda self,other: self.read() + other
    __sub__ = lambda self,other: self.read() - other
    __mul__ = lambda self,other: self.read() * other
    __div__ = lambda self,other: self.read() / other
    __mod__ = lambda self,other: self.read() % other
    __pow__ = lambda self,other: self.read() ** other
    __neg__ = lambda self,other: -self.read()
    __lt__ = lambda self,other: self.read() < other
    __gt__ = lambda self,other: self.read() > other
    __le__ = lambda self,other: self.read() <= other
    __ge__ = lambda self,other: self.read() >= other
    __eq__ = lambda self,other: self.read() == other
    __ne__ = lambda self,other: self.read() != other
    __and__ = lambda self,other: self.read() & other
    __xor__ = lambda self,other: self.read() ^ other
    __or__ = lambda self,other: self.read() | other
    __lshift__ = lambda self,other: self.read() << other
    __rshift__ = lambda self,other: self.read() >> other

    __radd__ = lambda self,other: other + self.read()
    __rsub__ = lambda self,other: other - self.read()
    __rmul__ = lambda self,other: other * self.read()
    __rdiv__ = lambda self,other: other / self.read()
    __rmod__ = lambda self,other: other % self.read()
    __rand__ = lambda self,other: other & self.read()
    __rxor__ = lambda self,other: other ^ self.read()
    __ror__ = lambda self,other: other | self.read()

    __iadd__ = lambda self,other: self.write(self.read() + other)
    __isub__ = lambda self,other: self.write(self.read() - other)
    __imul__ = lambda self,other: self.write(self.read() * other)
    __idiv__ = lambda self,other: self.write(self.read() / other)
    __imod__ = lambda self,other: self.write(self.read() % other)
    __ipow__ = lambda self,other: self.write(self.read() ** other)
    __iand__ = lambda self,other: self.write(self.read() & other)
    __ixor__ = lambda self,other: self.write(self.read() ^ other)
    __ior__ = lambda self,other: self.write(self.read() | other)
    __ilshift__ = lambda self,other: self.write(self.read() << other)
    __irshift__ = lambda self,other: self.write(self.read() >> other)

    iadd = __iadd__
    isub = __isub__
    imul = __imul__
    idiv = __idiv__
    imod = __imod__
    ipow = __ipow__
    iand = __iand__
    ixor = __ixor__
    ior = __ior__
    ilshift = __ilshift__
    irshift = __irshift__

    store_in_mem = lambda self,address: self.read().store_in_mem(address)

class MemValue(_mem):
    __slots__ = ['last_write_block', 'reg_type', 'register', 'address', 'deleted']

    def __init__(self, value):
        self.last_write_block = None
        if isinstance(value, int):
            self.value_type = regint
            value = regint(value)
        elif isinstance(value, MemValue):
            self.value_type = value.value_type
        else:
            self.value_type = type(value)
        self.reg_type = self.value_type.reg_type
        self.address = program.malloc(1, self.reg_type)
        self.deleted = False
        self.write(value)

    def delete(self):
        program.free(self.address, self.reg_type)
        self.deleted = True

    def check(self):
        if self.deleted:
            raise CompilerError('MemValue deleted')

    def read(self):
        self.check()
        if program.curr_block != self.last_write_block:
            self.register = library.load_mem(self.address, self.value_type)
            self.last_write_block = program.curr_block
        return self.register

    def write(self, value):
        self.check()
        if isinstance(value, MemValue):
            self.register = value.read()
        elif isinstance(value, (int,long)):
            self.register = self.value_type(value)
        else:
            self.register = value
        if not isinstance(self.register, self.value_type):
            raise CompilerError('Mismatch in register type, cannot write \
                %s to %s' % (type(self.register), self.value_type))
        library.store_in_mem(self.register, self.address)
        self.last_write_block = program.curr_block
        return self

    def reveal(self):
        if self.register.is_clear:
            return self.read()
        else:
            return self.read().reveal()

    less_than = lambda self,other,bit_length=None,security=None: \
        self.read().less_than(other,bit_length,security)
    greater_than = lambda self,other,bit_length=None,security=None: \
        self.read().greater_than(other,bit_length,security)
    less_equal = lambda self,other,bit_length=None,security=None: \
        self.read().less_equal(other,bit_length,security)
    greater_equal = lambda self,other,bit_length=None,security=None: \
        self.read().greater_equal(other,bit_length,security)
    equal = lambda self,other,bit_length=None,security=None: \
        self.read().equal(other,bit_length,security)
    not_equal = lambda self,other,bit_length=None,security=None: \
        self.read().not_equal(other,bit_length,security)

    pow2 = lambda self,*args,**kwargs: self.read().pow2(*args, **kwargs)
    mod2m = lambda self,*args,**kwargs: self.read().mod2m(*args, **kwargs)
    right_shift = lambda self,*args,**kwargs: self.read().right_shift(*args, **kwargs)

    bit_decompose = lambda self,*args,**kwargs: self.read().bit_decompose(*args, **kwargs)

    if_else = lambda self,*args,**kwargs: self.read().if_else(*args, **kwargs)

    def __repr__(self):
        return 'MemValue(%s,%d)' % (self.value_type, self.address)


class MemFloat(_mem):
    def __init__(self, *args):
        value = sfloat(*args)
        self.v = MemValue(value.v)
        self.p = MemValue(value.p)
        self.z = MemValue(value.z)
        self.s = MemValue(value.s)

    def write(self, *args):
        value = sfloat(*args)
        self.v.write(value.v)
        self.p.write(value.p)
        self.z.write(value.z)
        self.s.write(value.s)

    def read(self):
        return sfloat(self.v, self.p, self.z, self.s)

class MemFix(_mem):
    def __init__(self, *args):
        arg_type = type(*args)
        if arg_type == sfix:
            value = sfix(*args)
        elif arg_type == cfix:
            value = cfix(*args)
        else:
            raise CompilerError('MemFix init argument error')
        self.reg_type = value.reg_type
        self.v = MemValue(value.v)

    def write(self, *args):
        if self.reg_type == 's':
            value = sfix(*args)
        else:
            value = cfix(*args)
        self.v.write(value.v)

    def read(self):
        if self.reg_type == 's':
            return sfix(self.v)
        else:
            return cfix(self.v)

def getNamedTupleType(*names):
    class NamedTuple(object):
        class NamedTupleArray(object):
            def __init__(self, size, t):
                import types
                self.arrays = [types.Array(size, t) for i in range(len(names))]
            def __getitem__(self, index):
                return NamedTuple(array[index] for array in self.arrays)
            def __setitem__(self, index, item):
                for array,value in zip(self.arrays, item):
                    array[index] = value
        @classmethod
        def get_array(cls, size, t):
            return cls.NamedTupleArray(size, t)
        def __init__(self, *args):
            if len(args) == 1:
                args = args[0]
            for name, value in zip(names, args):
                self.__dict__[name] = value
        def __iter__(self):
            for name in names:
                yield self.__dict__[name]
        def __add__(self, other):
            return NamedTuple(i + j for i,j in zip(self, other))
        def __sub__(self, other):
            return NamedTuple(i - j for i,j in zip(self, other))
        def __xor__(self, other):
            return NamedTuple(i ^ j for i,j in zip(self, other))
        def __mul__(self, other):
            return NamedTuple(other * i for i in self)
        __rmul__ = __mul__
        __rxor__ = __xor__
        def reveal(self):
            return self.__type__(x.reveal() for x in self)
    return NamedTuple

import library
