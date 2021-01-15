import gdb

class BinaryTree(gdb.Command):
    def __init__(self):
        super(BinaryTree, self).__init__("binary-tree", gdb.COMMAND_OBSCURE)

    def invoke(self, arg, from_tty):
        print('| `value` | `fun7` | `addr`       | `left_addr`  | `right_addr` |')
        print('| ------- | ------ | ------------ | ------------ | ------------ |')
        self.show_tree(int(arg, 16))

    def show_tree(self, addr, fun7=0, level=0):
        addr       = gdb.Value(addr).cast(gdb.lookup_type('int').pointer())
        value      = (addr + 0).dereference()
        left_addr  = (addr + 1).dereference()
        right_addr = (addr + 2).dereference()

        fields = [ ''
                 , ' {:<5} '.format(int(value))
                 , ' 0x{:<2x} '.format(int(fun7))
                 , ' 0x{:0<8x} '.format(int(addr))
                 , ' 0x{:0<8x} '.format(int(left_addr))
                 , ' 0x{:0<8x} '.format(int(right_addr))
                 , '' ]
        print('|'.join(fields))
        if not left_addr == 0:
            self.show_tree(left_addr, fun7, level+1)
        if not right_addr == 0:
            self.show_tree(right_addr, 2**level + fun7, level+1)

BinaryTree()
