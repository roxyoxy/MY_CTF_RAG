# justctf-2025_private~2

# slowrun

- optimize-me challenge
- challenge is to calculate `f(13337)`
```python
def f(x):
    if x == 0:
        return 2
    
    if x <= 1:
        return 1
    
    return g(x-1) + 73 * x ** 5 + 8 * x ** 3 + x - 4

def g(x):
    if x <= 1:
        return 1
    return f(x-1) + 3 * f(x-2) - 5 * f(x-3) + 3 * x ** 4
```

## solution
- reverse binary to reconstruct original equations
- rewrite equations using func caching or use wolfram alpha to solve them
