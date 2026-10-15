# scriptctf-2025_Auction

# Auction | NoobMaster

- Description: Just win the auction. 

auction.zip: /attachments/auction.zip

# Write-up

Vulnerability 1: By funding the NoobMaster PDA before it is initialized (via `initialize_bidder`), the PDA would get an already initialized error (because of the lamports check). This causes NoobMaster's bid to fail, since the data expected by the `bid` function is not written on the PDA.

However, we are still not able to bid anything, because NoobMaster PDA has not bid yet.

Vulnerability 2: Call initialize again, with non-canonical bumps (for vault and winner PDAs). This allows us the call to `initialize()` to pass, setting us as the first bidder, and we can bid!

Note: While bidding, the winner PDA provided should be the one with canonical bump, because that's the one the server checks for.

Solve contract and script at `/src/solve/`

# Flag - scriptCTF{7h3_0nly_b1dd3r}

