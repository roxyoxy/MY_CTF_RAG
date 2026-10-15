# htb-university-2025_deadroute

![img](../../assets/banner.png)

<img src='../../assets/htb.png' style='zoom: 80%;' align=left /> <font size='10'>DeadRoute</font>

2<sup>nd</sup> December 2025

Prepared By: `dimasmaulana`

Challenge Author(s): `dimasmaulana`

Difficulty: <font color='green'>Easy</font>

<br><br>

# Synopsis

DeadRoute is a easy web challenges that enforce user to exploit a race condition in the custom Go router implementation to bypass IP-based access control and retrieve an authentication token. Using the obtained token, the user must exploit a path traversal vulnerability in the note reading functionality to retrieve the flag from the root directory.s

## Description

* Welcome to the Tinselwick Sticky Notes board! This is the official digital bulletin board for the town of Tinselwick. Residents use this board to post updates, reminders, festive greetings, and a special section for New Year's resolutions. Unfortunately, the Town Clerk, in a rush to secure the board against the meddling of the Grinch, implemented a new routing system that has locked everyone out of the admin panel—including themselves! This is part of the Grinch’s plot to spread 'paywall of spells' logic into the town's systems, which will end sharing and lock joy behind a cost. The system responsible for making the town's wishes come true is also affected, meaning residents are now blocked from submitting new wishes and the current ones won't become true. We need you to find a way around the blocked routes to retrieve the emergency override code so the festivities can continue as planned and stop the spread of the corrupted festive code.

## Skills Required

- Go (Golang) Code Analysis
- Race Conditions
- Path Traversal
- Web Exploitation

## Skills Learned

- Identifying race conditions in shared slice usage in Go.
- Bypassing non-recursive input sanitization.
- Writing concurrent exploit scripts in Go.

# Enumeration

## Analyzing the source code

We are provided with the source code of the application. The structure is a standard Go web app:

- `src/main.go`: Entry point.
- `src/routes.go`: Route definitions.
- `src/models/router.go`: Custom router implementation.
- `src/models/middleware.go`: Middleware definitions (Auth, LocalHostOnly, etc.).
- `src/controllers/`: Handlers for the endpoints.

When we run the attachment, we can see therese multiple endpoints:

![alt text](assets/main_endpoint.png)
![alt text](assets/login.png)
![alt text](assets/admin.png)

### Route Analysis

In `src/routes.go`, we see the following interesting routes:

```go
// Protected admin routes
r.Get("/admin/login-token", models.LocalHostOnly, http.HandlerFunc(adminController.LoginToken))

// Note operations with ID via query parameter
r.Get("/admin/notes/read", models.RequireAuth, http.HandlerFunc(noteController.ReadNote))
```

The `/admin/login-token` endpoint is protected by `models.LocalHostOnly` middleware, which checks if the request comes from `127.0.0.1` or `::1`.

The `/admin/notes/read` endpoint requires authentication (`models.RequireAuth`) and takes an `id` parameter.

# Solution

## Finding the vulnerability

### Vulnerability 1: Race Condition in Router

In `src/models/router.go`, the `Router` struct maintains a list of global middleware `mws`.

```go
type Router struct {
	mu     sync.RWMutex
	routes map[string]http.HandlerFunc
	mws    []Middleware
}
```

When a route is registered (e.g., via `Get`), the handler is wrapped with the middleware. However, the implementation of how middleware is applied during request handling (or rather, how it seems to be constructed) has a flaw.

Wait, looking closely at `router.go`:

```go
func (r *Router) Get(pattern string, h ...Handler) {
    r.mu.Lock()
    routeKey := "GET:" + pattern
    r.routes[routeKey] = func(w http.ResponseWriter, req *http.Request) {
        // ...
        r.mu.RLock()
        mws := r.mws
        r.mu.RUnlock()
        for _, handler := range h {
            mws = append(mws, getMWFromHandler(handler))
        }
        // ...
    }
    r.mu.Unlock()
}
```

The `mws := r.mws` line inside the closure copies the slice header. `r.mws` is the slice of global middleware.
When `append(mws, ...)` is called, if `r.mws` has enough capacity in its underlying array to accommodate the new elements, `append` will write to the **shared underlying array**.

Since `r.mws` is shared across all requests, and multiple requests can run concurrently, this creates a race condition.

If Request A (to `/`) and Request B (to `/admin/login-token`) run simultaneously:
1. Both read `r.mws`.
2. Request B appends `LocalHostOnly` to the slice. This writes to `backing_array[len(r.mws)]`.
3. Request A appends its handler (or nothing/other middleware) to the slice. This *also* writes to `backing_array[len(r.mws)]`.

If Request A overwrites the `LocalHostOnly` middleware that Request B just wrote (or was about to read/use), Request B might proceed without the `LocalHostOnly` check effectively applied (or with a different handler in its place that doesn't block access).

This allows an attacker to bypass the `LocalHostOnly` check by spamming requests to a public endpoint (like `/`) while simultaneously requesting `/admin/login-token`.

### Vulnerability 2: Path Traversal in Note Controller

In `src/controllers/note_controller.go`, the `ReadNote` function handles the `id` parameter:

```go
func (c *NoteController) ReadNote(w http.ResponseWriter, r *http.Request) {
    // ...
    noteID := r.URL.Query().Get("id")
    // ...
    // Sanitize noteID to prevent directory traversal
    noteID = strings.ReplaceAll(noteID, "../", "")
    
    filePath := filepath.Join(c.notesDir, noteID)
    // ...
}
```

The sanitization `strings.ReplaceAll(noteID, "../", "")` is insufficient because it is not recursive. It only removes `../` once.
An attacker can construct a payload like `....//`.
`strings.ReplaceAll` sees `../` in the middle of `....//` (indices 2-4) and removes it, leaving `../`.

So, `....//....//flag.txt` becomes `../../flag.txt`.
Since the app runs in `/app` and notes are in `/app/notes`, `../../flag.txt` resolves to `/flag.txt`, which is where the flag is located.

## Exploitation

### Connecting to the server

We can write a Go script (`solve.go`) to exploit these vulnerabilities.

### Getting the flag

The exploit strategy is:
1.  **Race**: Launch two concurrent workers.
    *   One spams requests to `/` to trigger the slice append race.
    *   The other spams requests to `/admin/login-token` to try and catch a moment where the IP check is bypassed.
2.  **Token**: Once `/admin/login-token` returns the token (a 64-char hex string), capture it.
3.  **Traversal**: Use the token to authenticate and request `/admin/notes/read?id=....//....//flag.txt`.

The provided `htb/solve.go` implements this:

```go
// ...
func (api *API) runSlashSpam(...) {
    // Spams GET /
}

func (api *API) runLoginTokenSpam(...) {
    // Spams GET /admin/login-token
}

func (api *API) readFlag(token string) {
    get, err := api.client.Get(api.baseURL + "/admin/notes/read?id=....//....//flag.txt")
    // ...
}
// ...
```

Running the solver:

```bash
go run htb/solve.go
```

Output:
```
Token found: <32-byte-hex-token>
HTB{xxxxxxx}
```