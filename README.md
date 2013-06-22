VStats
======

Tiny PHP metrics extension.

What?
-----

At various points through your application, you can submit any basic data which is sent via a local UDP 
packet. You can then setup a simple listening script which collects and buffers these packets for a short amount
of time before sending the collated packets to a remote location. This remote can then process the obtained information
any way you want.

Each packet is given it's own label so you can categorise the data later on. The data is also key-valued for 
further categorisation.

Currently, the data in the packet is sent URL encoded for speed of development. It is likely JSON encoding will be 
implemented as well in the future.


Why?
----

Lightweight, effecient and decoupled method of collecting a massive amount of stats across your entire application. 

Since you can send any data you wish, it can be as flexible as you require it to be.


Installation
------------

```
git clone https://github.com/MrLunar/VStats vstats
cd vstats
phpize
./configure
make
make test
```

Then copy `modules/vstats.so` to your extension directory (`php -i | grep extension_dir`).

Your application is now ready to use the built-in functions to send basic data packets. Indeed, all that would happen 
right now is that the packets get lost in the ether because something needs to receive them. 

Receiver
--------

The separate receiver means that only the bare essential processing is carried out during the request itself. 
All the receiver should do is save the packets for a few seconds before sending them off to a remote server.

An example implementation can be found here: https://gist.github.com/MrLunar/5840723

All you need to do is collect the submitted POST to your remote server, and save them in a database
(NoSQL is highly recommended here, I use MongoDB).

Configuration
-------------

VStats accepts a few php.ini settings.

```
; Extremely useful to identify the server. The label of every packet is prefixed with this.
vstats.default_prefix = "server_name"

; For every request, a unique ID is generated and sent with every packet during the request.
vstats.send_request_uuid = 1

; Send some stats for each request under the label "request_stats"
vstats.send_request_peak_mem_usage
vstats.send_request_time
```
