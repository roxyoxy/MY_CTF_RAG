# blockchains ticket master solution README

# Ticket Master Writeup

The function `equal_slices_bits` only checks whether the data parts of two slices coinside. While in TON, the address used for internal transfer has two encoding formats: `addr_std` and `addr_var`.

Therefore, we can use the coin minter address encoded in `addr_var` to bypass the below check and have the coin minter mint any amount of coins for the exploit contract's account.

> `addr_var` in `dest` of internal messages are automatically replaced with `addr_std`.

```
throw_if(error::invalid_target, equal_slices_bits(recipient, account::coin_minter));
```

When using tickets to exchange for a prize, if no payload is specified, the default message will be used; otherwise, we can have the service counter to send arbitrary message to any address. That means, we can let the service counter send a message to the ticket minter to mint enough tickets for the exploit contract's account to exchange for the flag XD

```
slice response_address = account_owner;
cell forward_payload = null();
if payload.cell_null?() {
    forward_payload = begin_cell()
        .store_op(op::message)
        .store_slice("Successfully exchanged the item")
        .store_uint(item_id, ITEM_ID_SIZE)
        .end_cell();
} else {
    slice payload = payload.begin_parse();
    response_address = payload~load_msg_addr();
    forward_payload = payload~load_ref();
}
```

<div style="background:#f6e3bc;border-radius:1rem;padding:1rem"><b>❖ Note</b><br>
This challenge uses an older version of <code>@ton/sandbox</code> and <code>@ton/core</code> in order to support features from before global version 10.
<br />
Since global version 10, <code>addr_var</code> are not allowed in <code>dest</code> of external messages. Additionally, <code>LDMSGADDR(Q)</code>, <code>PARSEMSGADDR(Q)</code> no more support <code>addr_var</code>.
</div>
