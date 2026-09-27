# SQL injection in the login form

The login endpoint concatenates the username parameter straight into the
query, so a classic injection bypasses authentication:

    username: admin' --
    password: anything

Behind a WAF the payload needs obfuscation, for example inline comments
or char(). The exploit.py script automates the blind extraction of the
admin password hash with a binary search on the response time.

Defense: prepared statements, or an ORM that binds parameters. Never
build SQL by string concatenation of user input.
