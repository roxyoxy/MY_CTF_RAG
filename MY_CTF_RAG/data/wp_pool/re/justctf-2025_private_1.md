# justctf-2025_private~1

## 6pack

- chat using ipv6 flowlabel field as transport
- go have removed `main_*` symbols
- player gets .pcap with traffic records
- traffic record contains base64 encoded windows PE file
- PE file is a flag checker
- PE loads section (shellcode) .go.runtimeinfo from original 6pack binary
- PE decrypts loaded data using rc4 - key is passed as cmd arg
- there are 1024 possibilites for key - player has to brute it to produce valid shellcode
- once done, shellcode is executed
- shellcode checks flag length
- shellcode "decyrpts" itself
- it reverses the flag in place
- it chunks flag into 3byte chunks
- for each chunk it loads winapi `Crypt*` functions to calculate sha256
- compares hashes against hashes stored in shellcode
- sc returns to the PE file wheter flag was valid or no


how to run the binary:
```
docker network create --ipv6 ip6net
docker run --rm -it --network ip6net temp
```