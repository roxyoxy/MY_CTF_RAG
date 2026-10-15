# pearlctf-2025_pearlinthesky__solve

## Writeup PearlCTF (2025): Pearl in the Sky

### Overview

The minecraft world contains a typical skyblock setup. However the player can easily spot the huge amount of barrels right under them. We can easily deduce that in this world one of the barrels must contain the diamond as there seems no other way to get it.

### The Task

The task is to look for a diamond. On inspection we find that the barrels are all filled with coal. One of these barrels must contain a minecraft:diamond. The player can either look for it manually or write a script to automate the task.

### Solution

We setup a repeating command block with `needs redstone` set to `True` connected with a redstone clock with the following command:

```/execute at @a if block ~ ~ ~ minecraft:barrel unless block ~ ~ ~ minecraft:barrel{Items:[{id:"minecraft:diamond", tag:{}}]} run setblock ~ ~ ~ air replace```

This executes a command that replaces the block at the coordinates where the player is standing to air if it does not contain a diamond.

Now switch to spectator mode and fly through the barrels to check them all.

We can also setup another one to change player position accordingly when we reach the end to automate the search

Setup a simlar command block but this time with

```execute at @a if entity @a[z=60,dx=0,dy=0,dz=0] run tp @a ~1 ~ 0```

This will send the player to z=0 and increment the x coordinate whenever we reach z=60.

The player can hold W and and fly through all the barrels at high speeds making them disappear if it does not contain a diamond. 

We open the barrel that does not disappear when passing through it. The diamond inside it contains the flag string. Enclose it within pearl{}
