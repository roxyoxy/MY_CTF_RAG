# scriptctf 2025 ForeignDesign

# ForeignDesign | Ashray Shah
- Description: Java is fun, but sometimes I crave more.

ForeignDesign.jar:attachments/ForeignDesign.jar

# Writeup

The `NativeLoader` class in the Java takes the file native that is stored as a resource inside the JARfile and unzips it. Inside, you will find the folders:
```
/linux64
/linux32
/win32
/macos
```
Containing the native JNI library for each platform.

After reversing both the Java and the C code, you can figure out that the Java main function calls `initialize()`, which is a native method defined in the binary. The initialize function sets the value of the `ll` and `lll` java variables to:

```java
public static int[] ll = {32, 92, 4, 104, 106, 76, 96, 113, 42, 65, 22, 43, 203, 84, 220, 98, 210, 71, 29, 123, 20, 125, 199};
public static int[] lll = {76, 230, 117, 243, 84, 54, 103, 197, 104, 251, 83, 253, 128, 159};
```

After which, user input is taken, and the Java method `ck` is called. After checking the length against the `ll` and `lll` variables, this function goes letter by letter, manipulating the index:
```java
int idx = (i * 5 + 3) % (ws.length());
```
Getting the char at the position `idx`:
```java
char ch = ws.charAt(idx);
```
And finally running the C method `sc(ch, i)`.

For each index, if the value is a multiple of 2 (`i % 2 == 0`), it is passed through the C method `s1(ch, i)`, otherwise the Java method `s2(ch, i)`. Finally, the output of either function is checked with either a value from the `ll` or `lll` arrays, depending on if `i` is larger than the size of `ll`.

```java
if (i < 23)
{
    arrItem = ll[i];
}
else 
{
    arrItem = lll[i - 23];
}
```

With this, we can analyze s1 and s2, and realize that s1 is inversible, while s2 requires a bytewise brute-force due it containing conditionals. You are also able to combine `ll` and `lll` into one array, reverse s1, and make the bruteforce for s2.

Look in _Decoder.java_ for automated solution.

# Flags: scriptCTF{nO_MOr3_n471v3_tr4N5l471on}