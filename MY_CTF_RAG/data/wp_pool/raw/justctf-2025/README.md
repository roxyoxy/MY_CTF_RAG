# README

# justCTF 2025

This repo contains sources for [justCTF 2025](https://2025.justctf.team/) challenges hosted by [justCatTheFish](https://ctftime.org/team/33893) as well as summary of winners and sponsors of the event.

TLDR: Run a challenge with `./run.sh` (requires Docker/docker-compose and might require `sudo` as we use `nsjail` extensively under the hood).

The [`challenges/`](./challenges/) contains challanges directories with the following structure:
* `README.md` - official challenge description used during CTF
* `public/` - files that were public/to download
* `private/` - sources and other unlisted files
* `private/run.sh` - shell script to run the challenge locally (uses Docker and sometimes docker-compose)
* `private/flag.txt` - the flag (don't look there?)
* `private/metadata.json` - challenge metadata
* `private/solve.sh`/`private/solver/` - scripts and files with raw solution (not present for every challenge)
* other files


### Winners & Prizes
* 1st place - [PIG SEK@1](https://ctftime.org/team/169557) - $4096 + 3x IDA Pro Expert-4 Licenses + Swag
* 2nd place - [infobahn](https://ctftime.org/team/364723) - $2048 + 1x IDA Pro Expert-4 License + Swag
* 3rd place - [kalmarunionen](https://ctftime.org/team/114856) - $1024 + 1x IDA Pro Expert-4 License
* 4th place - [Gimel](https://ctftime.org/team/357179) - $768
* 5th place - [Never Stop Exploiting](https://ctftime.org/team/13575) - $512
* 6th place - [organizers](https://ctftime.org/team/42934) - $256

### justCTF 2025 sponsors:
* Trail of Bits - https://www.trailofbits.com/
* OtterSec - https://osec.io/
* HEX-RAYS - https://hex-rays.com/
* Zellic - https://zellic.io/

Thanks again to all the sponsors who made this event possible!

### Challenges

(Sorted from most solved to least solved)

| Category         | Name                | Points | Solves |
|------------------|---------------------|--------|--------|
| Re, Baby         | baby-goes-re        | 55     | 186    |
| Pwn, Baby        | shellcode printer   | 114    | 110    |
| Misc, Baby       | Baby Audio          | 143    | 85     |
| Misc, Re         | Satellite           | 155    | 76     |
| Re               | slowrun             | 163    | 71     |
| Blockchain, Baby | Otternaut Launch    | 174    | 64     |
| Pwn, Baby        | Baby Heap           | 176    | 63     |
| Misc             | PyEvaline crimes    | 181    | 60     |
| Misc, Baby       | Baby SUID           | 201    | 50     |
| Web, Baby        | Positive Players    | 256    | 30     |
| Pwn              | prospector          | 256    | 30     |
| Blockchain       | Otternaut Syndicate | 271    | 26     |
| Pwn              | jCTFcoin            | 271    | 26     |
| Re, Misc         | 6pack               | 303    | 19     |
| Web              | Ticket Master       | 303    | 19     |
| Misc             | QSecureByRejection  | 326    | 15     |
| Pwn              | tape                | 420    | 5      |
| Web              | Notes               | 435    | 4      |
| Blockchain       | Otternaut Exodus    | 435    | 4      |
| Re               | java                | 435    | 4      |
| Crypto, Web      | HTTP Signing        | 435    | 4      |
| Web              | Vault Tec           | 500    | 1      |
| Web              | Simple Tasks        | 500    | 1      |
| Misc, Crypto     | Casino              | 500    | 1      |
| Web              | Busy Traffic        | 500    | 0      |

### Write-ups
Write-ups created by players can be found on [CTFTime](https://ctftime.org/event/2711/tasks/) as well as on [our discord](https://discord.gg/phyqdh6). 
You should also look at challenges solution directories, if they exist (`solver.sh`/`solver/`).
