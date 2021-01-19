# Solution

Each level of the bomb is defused by passing specific data into a buffer
to cause an overflow.

The exploit files used for level `i` are stored at
`exploits/exploit_i.txt`. They have to be converted to raw bytes using
some tool like `hex2raw` (provided with the handout) before being
passed to the bomb.
Note that these exploits are specific to the user id `caleb`.
You can use the `exploit` program as demonstrated below to generate the
binary exploit strings for an arbitrary user id.

Descriptions of the constructions of the exploits follow:

## Level 0

In order to jump to `smoke`, we need to place the address of `smoke` on
the stack where `ret` looks for a return address.
Disassembling `getbuf`, we see that the buffer is located at `ebp-0x28`.
The old `ebp` was stored on the stack at the start of `getbuf`, so the
return address is located at `ebp+0x8`. In total, we need
`0x28+0x8-0x4=44` bytes of padding before the address of `smoke`.

Perform this exploit from the `handout` directory with
```
cat ../solution/exploits/exploit_0.txt | ./hex2raw | ./bufbomb -u caleb

OR

./makecookie <user id> | ../solution/exploits/exploit bufbomb 0 | ./bufbomb -u <user id>
```

## Level 1

We can use the overflow to jump to `fizz` in an analogous manner to
jumping to `smoke`. If `call fizz` had been executed, the argument
and a return address would have been push onto the stack first, in that
order. These would occupy the positions on the stack just below our
faked `fizz` address. We don't care about the return address from
`fizz`, so we put 4 bytes of padding and then whatever value we want for
the argument, in this case, the cookie.

Perform this exploit from the `handout` directory with
```
cat ../solution/exploits/exploit_1.txt | ./hex2raw | ./bufbomb -u caleb

OR

./makecookie <user id> | ../solution/exploits/exploit bufbomb 1 | ./bufbomb -u <user id>
```

## Level 2

Using **GDB** we can determine the address of the start of the buffer
and use that in place of `smoke`/`fizz` in the previous examples.
We then replace the padding we had been putting in the buffer with the
binary for the following assembly:
```
mov eax, 0x7356a731
mov ecx, <address of global_value>
mov [ecx], eax
push <address of bang>
ret
```
(the addresses for `global_value` and `bang` are straightforward to
obtain, for example using **GDB**)

Perform this exploit from the `handout` directory with
```
cat ../solution/exploits/exploit_2.txt | ./hex2raw | ./bufbomb -u caleb

OR

./makecookie <user id> | ../solution/exploits/exploit bufbomb 2 | ./bufbomb -u <user id>
```

## Level 3

In this case, we simply want to return the cookie from `getbuf`,
which is as simple as setting `eax` to the cookie in our exploit code,
but we need to ensure that the *canary value* can still be checked.
For this, we must ensure that the stack pointer in `test` is set
correctly after returning from `getbuf`, so that the variable `local` is
accessed appropriately. The bomb is kindly set up so that the stack is
the same each run, so we can simply sample `esp`/`ebp` in **GDB**
and hard-code in the appropriate values.
The payload assembly then looks something like this:
```
mov eax, 0x7356a731
push 0x8048dbe
ret
```

Perform this exploit from the `handout` directory with
```
cat ../solution/exploits/exploit_3.txt | ./hex2raw | ./bufbomb -u caleb

OR

./makecookie <user id> | ../solution/exploits/exploit bufbomb 3 | ./bufbomb -u <user id>
```

## Level 4

This level is more or less the same as level 3.
The difference is there is a range of addresses at which the buffer may
start, as opposed to a single, known address.
This means, on any given run, we don't know
  * what value needs to be restore for the pushed `ebp`
  * what address to jump to
To accommodate this, we sample the buffer starting address several
times, take the maximum, and fill the majority of the buffer with `NOP`
instructions (a `NOP` slide). We also add code to the exploit assembly
to compute the pushed `ebp` value and restore it to the `ebp` register,
like `leave` would in a normal run of the program.
The payload assembly then looks something like this:
```
mov eax, 0x7356a731
lea ebp, [esp+0x28]
push 0x8048e3a
ret
```

Perform this exploit from the `handout` directory with
```
cat ../solution/exploits/exploit_4.txt | ./hex2raw -n 5 | ./bufbomb -u caleb -n

OR

./makecookie <user id> | ../solution/exploits/exploit bufbomb 4 | ./bufbomb -u <user id> -n
```
