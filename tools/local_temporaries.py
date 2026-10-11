"""Keep proven instruction-local P-code unique storage in immutable C++ locals.

Unique space is a translator scratch array, not the PowerPC register file or
guest memory. This lowering preserves architectural state and exceptions, but
does not materialize the original instruction's dead scratch bytes for snapshots.
Use the original lowering for consumers that explicitly compare scratch bytes.
"""
from dataclasses import dataclass


@dataclass(frozen=True)
class _Byte:
    name: str
    size: int
    index: int


class LocalTemporaries:
    """Read/write callbacks for one instruction, enclosed in its own C++ block.

    Construct through for_instruction before emitting code. A None result means
    the entire instruction must retain its existing State.unique lowering.
    Emit operations in their original order using these callbacks. Each write
    evaluates its expression immediately and only then replaces byte ownership;
    later reads cannot accidentally reevaluate a changed register or memory load.
    """
    unique_size = 0x45000

    @classmethod
    def for_instruction(cls, operations, read, write):
        try:
            cls.require_instruction_local_unique(operations)
        except ValueError:
            return None
        return cls(read, write)

    @classmethod
    def require_instruction_local_unique(cls, operations):
        """Reject instructions whose scratch lifetime cannot be proved local.

        When enabling local storage for a complete program, audit every selected
        instruction first and fail generation if this rejects one. Falling back
        for a cross-instruction scratch reader cannot restore writes discarded
        by an earlier optimized instruction. for_instruction's None fallback is
        useful for an isolated lowering or a globally disabled optimization.
        """
        defined = set()
        try:
            for operation in operations:
                name, output, *inputs = operation
                # Internal jumps can skip definitions or revisit an SSA write.
                # Transfers out of this instruction cannot reenter its locals.
                if name in ("BRANCH", "CBRANCH", "CALL") and inputs[0][0] == "const":
                    raise ValueError("Internal P-code flow prevents local scratch proof")
                for node in inputs:
                    if node is not None and node[0] == "unique":
                        extent = cls._extent(node)
                        if not extent <= defined:
                            raise ValueError("Unique input lacks a same-instruction definition")
                if output is not None and output[0] == "unique":
                    defined.update(cls._extent(output))
        except (TypeError, IndexError) as error:
            raise ValueError("Malformed P-code operation") from error

    @classmethod
    def _extent(cls, node):
        _, offset, size = node
        offset = int(offset)
        if size not in (1, 2, 4, 8) or offset < 0 or offset + size > cls.unique_size:
            raise ValueError("Unsupported unique-space extent")
        return set(range(offset, offset + size))

    def __init__(self, read, write):
        self._original_read = read
        self._original_write = write
        self._bytes = {}
        self._next = 0

    def read(self, node):
        if node[0] != "unique":
            return self._original_read(node)
        self._extent(node)
        _, offset, size = node
        offset = int(offset)
        try:
            owners = [self._bytes[offset + index] for index in range(size)]
        except KeyError as error:
            raise ValueError("Unique input read before its original write") from error
        terms = []
        begin = 0
        while begin < size:
            owner = owners[begin]
            end = begin + 1
            while end < size and owners[end] == _Byte(owner.name, owner.size,
                                                       owner.index + end - begin):
                end += 1
            width = end - begin
            term = f"std::uint64_t({owner.name})"
            right = 8 * (owner.size - owner.index - width)
            if right:
                term = f"({term} >> {right})"
            if width < owner.size:
                term = f"({term} & UINT64_C({(1 << (8 * width)) - 1}))"
            left = 8 * (size - end)
            if left:
                term = f"({term} << {left})"
            terms.append(term)
            begin = end
        return terms[0] if len(terms) == 1 else "(" + " | ".join(terms) + ")"

    def write(self, node, value):
        if node[0] != "unique":
            return self._original_write(node, value)
        self._extent(node)
        _, offset, size = node
        offset = int(offset)
        name = f"local_unique_{self._next}"
        self._next += 1
        for index in range(size):
            self._bytes[offset + index] = _Byte(name, size, index)
        # Reads retain uint64_t arithmetic as in read_be<N>; narrowing occurs
        # only here, at exactly the original State::wu<N> write boundary.
        return f"[[maybe_unused]] const std::uint{size * 8}_t {name} = std::uint{size * 8}_t({value});"
