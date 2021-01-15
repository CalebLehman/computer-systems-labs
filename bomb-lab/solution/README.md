# Solution

The file `input.txt` contains the passwords for 6 phases and the secret
phase, each on a separate line. There is also a script, `defuse`, which
can be used to generate a set of passwords with the command
`defuse [path to bomb]`.

The bomb itself is at `handouts/bomb`. Note that all debugging involving
running the bomb was done in **GDB** with a breakpoint set at the
`explode_bomb` function, to prevent the bomb from ever detonating.

## Overview

Disassembling `main`, it is clear that there are 6 phases to the bomb,
implemented by the corresponding functions `phase_X`.
Using `nm` to check the symbol table for functions with `phase` in the
name, we see that there is a `secret_phase` that we should go back and
find at the end.

## Phase 1

Disassembling `phase_1`, we see that it just compares the input string against
the string at `0x080497c0`. Examining this memory yields the string
`Public speaking is very easy.`

## Phase 2

Disassembling `phase_2`, we see that it starts by calling
`read_six_numbers` on the input string.
Disassembling `read_six_numbers`, we see that the reading is performed
by `sscanf` with the format string located at `0x08049b1b`.
Examining this memory with yields `"%d %d %d %d %d %d"`,
so the input should be six integers.

Back inside `phase_2`,
we see that the arguments of `read_six_numbers` are the input string and the location
`$ebp-0x18` on the stack, where the six integers are stored.
The first integer (at `$ebp-0x18`) is checked against `0x1`.
The next group of assembly lines implement a loop that checks the
current integer against the result of multiplying the previous integer
and a loop index (`$ebx + 1`).
In other words, the password `a[1] ... a[6]` must satisfy the following recurrence
```
a[1] = 1
a[i] = i * a[i-1]
```
which is equivalent to saying `a[i] = factorial of i`
Hence, the second password is `1 2 6 24 120 720`

## Phase 3

Disassembling `phase_3`, we see that it uses `sscanf` with a format
string at `0x080497de` and checks that more than `0x2` fields that are
parsed out. Examining the memory, we see that the format
string is `"%d %c %d"`, so the password is an integer, a character, and
an integer, which are stored at `$ebp-0xc`, `$ebp-0x5`, and `$ebp-0x4`,
respectively.

Next the routine checks that the first integer is less than or equal to
`0x7`. The conditional jump is performed with `ja`, so the comparison is
treated as unsigned. As a result, the only (signed) integers that pass
this check are `0, ..., 7`. The first integer is used to jump into some
kind of table which set expected values for the character and second
integer. We can use a short **GDB** command to pull the passwords from the
table/memory offsets:
```
define show_passwords
  printf "| First Integer | Character | Second Integer |\n"
  printf "| ------------- | --------- | -------------- |\n"
  set $i=0
  while ($i < 8)
    printf "| %d | %c | %d |\n", $i, *(char*)(*(int*)($i*4 + 0x080497e8)+1), *(short*)(*(int*)($i*4 + 0x080497e8)+5)
    set $i+=1
  end
end
```
which produces
| First Integer | Character | Second Integer |
| ------------- | --------- | -------------- |
| 0 | q | 777 |
| 1 | b | 214 |
| 2 | b | 755 |
| 3 | k | 251 |
| 4 | o | 160 |
| 5 | t | 458 |
| 6 | v | 780 |
| 7 | b | 524 |

Any of these rows will work.

## Phase 4

Disassembling `phase_4`, we see that it scans using the format string
at `0x08049808`, which upon inspection is `"%d"`.
This integer is checked to be greater than `0x0` and is passed to
`func4`.

Disassembling `phase_4`, we see that it is a recursive function.
Tracing its execution, we can derive the following rules:
```
func4(i) = 1                        for i <= 1
func4(i) = func4(i-1) + func4(i-2)  for i >  1
```
In other words, `func4` computes Fibonacci numbers (offset by one index
from the typical definition that sets `fib_0 = 0, fib_1 = 1`).
Back in `phase_4`, the result is checked against `0x37 = 55`,
so it is easy to figure out the password is `9`.

## Phase 5

Disassembling `phase_5`, we see that the length of the input string is
checked against `0x6`. A loop then runs over the input string, modifying
each character and storing in a new string. This new string is compared
against a string at `0x0804980b`, which we can inspect with `x/s` to
reveal `giants`.

Investigating the process used to generate the new string, we see that
the lowest 4 bits of each original character are used to index into the
16 bytes of memory at memory at `0x0804b220`.
Printing these bytes as characters with `printf "%.16s\n", 0x0804b220`
yields `isrveawhobpnutfg`.
Note that, since only the lower 4 bits of each character matters, there
are many valid passwords. For fun, we can easily generate a random
password (from a "nice-looking" subset of valid passwords)
with a short **Python** script.

```
#!/usr/bin/env python3
from random import choice

table   = 'isrveawhobpnutfg'
target  = 'giants'
choices = []
for i, c in enumerate(target):
    lo = table.index(c)
    choices.append(choice([ c
                            for hi in range(8)
                            for c in chr(lo+(hi<<4))
                            if c.isalnum() ]))
print(''.join(choices))
```

## Phase 6

Disassembling `phase_6`, we see that it reads 6 integers out of the
input.

The next section of `phase_6` loops over the 6 integers and for each
checks that
  * if you `dec`, it is less than or equal to `0x5`. The jump is
    implemented with the instruction `jbe`, so the comparison is
    unsigned, with the result being that only `1, ..., 6` are valid
  * it is not equal to any of the previous integers
Combining these two conditions, we see that it is simply checking that
the input is some permutation of `1 2 3 4 5 6`.

The next section of `phase_6` loops over the input and uses it to
traverse a linked list made of nodes which seem to have 3 fields, some
integer, a value from `1 2 3 4 5 6`, and a pointer to another node.
The ordering implied by our permutation of `1 2 3 4 5 6` is used to
order the nodes, which are then checked to be in decreasing order.
In any case, the location of the first node is loaded as `0x0804b26c`
and we can iterate through the nodes our self with a short **GDB** script
```
define show_linked_list
  set $node=0x0804b26c
  printf "|  i  | value |\n"
  printf "| --- | ----- |\n"
  while ($node != 0)
    printf "| %d | %d |\n", *(int*)($node + 4), *(int*)($node)
    set $node=*(long*)($node + 8)
  end
end
show_linked_list
```
to get the table
| `i` | `value` |
| --- | ----- |
| 1 | 253 |
| 2 | 725 |
| 3 | 301 |
| 4 | 997 |
| 5 | 212 |
| 6 | 432 |

The password is the indexes `i` sorted according to `value`, that is,
`5 1 3 6 2 4`.

## Secret Phase

As mentioned earlier, the symbol table contains an entry for the function
`secret_phase`. Disassembling the executable and searching for
`secret_phase`, we see that it is called from `phase_defused`,
a routine called after every `phase_X` function returns.

Disassembling `phase_defused`, we see that its body is skipped if less
than `0x6` input strings have been read, that is, it does nothing until
all 6 "non-secret" phases have been competed.
Next it uses `sscanf` to parse the string at `0x0804b770` with the
format string at `0x08049d03`. Placing a breakpoint at the `sscanf` call
and inspecting both strings shows that they are `"9"` and `"%d %s"`,
respectively. It seems that the 4th password is being re-parsed, this
time checking for a string after the integer. This string is later
compared to the string at `0x08049d09`, which can be examined to reveal
that it is `"austinpowers"`.

Disassembling `secret_phase`, we see that it reads a long integer, `i`, and
performs the check of `dec`ing the integer and comparing against `0x3e8 = 1000`.
The conditional jump is implemented
with `jbe`, so the comparison is unsigned and the valid inputs are
`1, ..., 1001`. This value and the memory address `0x0804b320` are
passed to `fun7` and the return value is compared to `0x7`.

We need to find an input that makes `fun7` return `0x7`. The search
space seems relatively small, so we can try simply checking every result with a
simple **GDB** session:
```
$ gdb [path to bomb]
break *secret_phase+44
commands
  silent
  set $ebx=$i
  continue
  end
break *secret_phase+66
commands
  silent
  set $i+=1
  printf "trying password = %d...\n", $i
  jump *secret_phase+44
  end
break *secret_phase+71
commands
  silent
  printf "\tpassword = %d is a success!\n", $i
  printf "\texiting...\n"
  quit
  end
set $i=0
set pagination off
set confirm off
r < [input-file] > /dev/null
```
This produces `1001` as the password.

For fun, we can investigate `fun7`
further to try and find a more revealing solution.
Disassembling `fun7`, we see that the memory at `0x0804b320` seems to
be the root of a binary search tree on which `fun7` searches for the input
value. Interpreting the assembly code, we can figure out that
  * each node of the tree has three fields, a value and two
    pointers to children
  * `fun7` is described by the following recursive definition:
```
fun7(node, input) = -1                                  if node == 0x0
fun7(node, input) = 0                                   if node->val == input
fun7(node, input) = 2 * func7(node->left,  input)       if node->val  < input
fun7(node, input) = 2 * func7(node->right, input) + 1   if node->val  > input
```
Since inputs not present in the tree return a negative value, the
password must be some value in the tree.
Any node in the tree can be represented as a string of `L` and `R` based
on the path to its location in the tree. For example, the root is
represented by the empty string, its left and right children are `L` and
`R`, respectively, their children are `LL`, `LR`, `RL`, `RR`, etc.
With this in mind, one way to interpret `fun7` is that it computes
the binary number obtained by taking the
string corresponding to `input` and making the replacements `L->0,
R->1` (treating the empty string as `0` as well).
This idea is implemented in the following **Python** code (**GDB** scripts
don't handle recursion well, due to the way it handles variables, so it
is more convenient to use the `gdb` module in **Python** to implement
this search)
```
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
```
which can be sourced in `gdb` and executed with `binary-tree 0x0804b320`
to produce
| `value` | `fun7` | `addr`       | `left_addr`  | `right_addr` |
| ------- | ------ | ------------ | ------------ | ------------ |
| 36    | 0x0  | 0x804b3200 | 0x804b3140 | 0x804b3080 |
| 8     | 0x0  | 0x804b3140 | 0x804b2e40 | 0x804b2fc0 |
| 6     | 0x0  | 0x804b2e40 | 0x804b2c00 | 0x804b29c0 |
| 1     | 0x0  | 0x804b2c00 | 0x00000000 | 0x00000000 |
| 7     | 0x4  | 0x804b29c0 | 0x00000000 | 0x00000000 |
| 22    | 0x2  | 0x804b2fc0 | 0x804b2900 | 0x804b2a80 |
| 20    | 0x2  | 0x804b2900 | 0x00000000 | 0x00000000 |
| 35    | 0x6  | 0x804b2a80 | 0x00000000 | 0x00000000 |
| 50    | 0x1  | 0x804b3080 | 0x804b2f00 | 0x804b2d80 |
| 45    | 0x1  | 0x804b2f00 | 0x804b2cc0 | 0x804b2840 |
| 40    | 0x1  | 0x804b2cc0 | 0x00000000 | 0x00000000 |
| 47    | 0x5  | 0x804b2840 | 0x00000000 | 0x00000000 |
| 107   | 0x3  | 0x804b2d80 | 0x804b2b40 | 0x804b2780 |
| 99    | 0x3  | 0x804b2b40 | 0x00000000 | 0x00000000 |
| 1001  | 0x7  | 0x804b2780 | 0x00000000 | 0x00000000 |

Again, we see that `1001` produces the desired value of `0x7` and is
therefore the password.
