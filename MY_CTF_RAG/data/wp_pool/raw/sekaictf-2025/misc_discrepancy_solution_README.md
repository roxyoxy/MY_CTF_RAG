# misc_discrepancy_solution_README

## Writeup

There are three native implementations of the Pickle Virtual Machine (PVM) in Python source code: the Python version, the C version, and the `pickletools` disassembler. The goal here is to find 5 different pickle payloads that lead to discrepancies in parsing between the implementation. 

* **Check 1** - only errors in `pickletools`
* **Check 2** - only errors in `pickle.py`
* **Check 3** - only errors in `_pickle.c`
* **Check 4** - only errors in `pickle.py` AND `_pickle.c`
* **Check 5** - only errors in `pickle.py` AND `pickletools`

In addition, each payload can only be a maximum of 8 bytes long. So they have to find 5 different payloads that causing pickle parsing discrepancies, AND each payload has to be very short.

The following are the solutions for each check:
* **Check 1** - `b'(\x88.'`
    * Pickletools complains if the stack is not empty after `STOP`, the actual unpicklers don't care
* **Check 2** - `b'\x88(e.'`
    * For the `APPENDS` opcode, all items on the stack until the `MARK` object are popped off. If there are **no** items between `MARK` and `APPENDS`, `_pickle.c` immediately moves on and pickletools doesn't care, but `pickle.py` will look for the `append` attribute in the top item before the `MARK` object. Therefore, if the top stack item doesn't have an `appends` attribute, it will error out only in `pickle.py`.
* **Check 3** - `b'F 5\n.'`
    * Whitespace in the `FLOAT` opcode causes an error in `_pickle.c` but the other two implementations don't care.
* **Check 4** - `b'(.'`
    * In pickletools, the `MARK` object counts as an actual value on the stack and can be returned from the unpickling process. In `_pickle.c` and `pickle.py`, `MARK` is only a "checkpoint" of sorts and is not actually a custom object pushed onto the stack.
* **Check 5** - `b'I1\x00\n.'`
    * Null bytes in opcodes like `INT` throw errors in `pickle.py` and pickletools (since `int('1\x00')` is invalid), but `_pickle.c` will stop processing at the null byte and successfully convert the values before the null byte into a number.

The solve is automated in `solve.py`.