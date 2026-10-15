# challenges_web_extra-safe-security-layers_private_private_description

# Extra safe security layers

Make use of `navigator.sendBeacon`,
Overwriting `res.user` via deconstructing `req.query`, thus changing the CSP policy:
```
if (req.query.text) {
    res.user = { ...res.user, ...req.query };
}
```
