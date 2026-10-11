"""Lower the four AltiVec user operations needed by Mac exception contexts.

Resolve by the exported language's user-op names, never by an assumed numeric
index. Unknown operations retain the generator's explicit unsupported stop.
"""


def lower_vector_intrinsic(op, userops, read, write):
    _, output, *inputs = op
    if not inputs or inputs[0][0] != 'const' or userops is None:
        return None
    name = userops.get(str(int(inputs[0][1])))
    supported = ('loadVectorElementWordIndexed', 'storeVectorElementWordIndexed',
                 'moveFromVectorStatusAndControlRegister', 'moveToVectorStatusAndControlRegister')
    if name not in supported:
        return None
    args = inputs[1:]

    def vector(node):
        return node is not None and node[0] == 'register' and node[2] == 16 \
            and 0x4200 <= int(node[1]) <= 0x43f0 and int(node[1]) % 16 == 0

    def base(node):
        # Ghidra's lvewx constructor exports r0 as an ordinary register input;
        # the ISA instead treats an encoded rA=0 as a constant zero address base.
        return 'UINT64_C(0)' if node == ['register', '0', 4] else read(node)

    def word(node):
        return node is not None and node[2] == 4 and node[0] in ('register', 'unique', 'const')

    if name == 'loadVectorElementWordIndexed' and vector(output) and len(args) == 2 and all(map(word, args)):
        return write(output, f'load_vector_word(s, std::uint32_t({base(args[0])}), std::uint32_t({read(args[1])}))')
    if name == 'storeVectorElementWordIndexed' and word(output) and len(args) == 3 \
            and vector(args[0]) and all(map(word, args[1:])):
        return write(output, f'vector_word_for_address({read(args[0])}, std::uint32_t({base(args[1])} + {read(args[2])}))')
    if name == 'moveFromVectorStatusAndControlRegister' and vector(output) and not args:
        return write(output, 'vector_status_register(s)')
    if name == 'moveToVectorStatusAndControlRegister' and output is None and len(args) == 1 and vector(args[0]):
        return f'move_to_vector_status(s, {read(args[0])});'
    raise ValueError(f'Malformed AltiVec context user operation: {name}')
