### HyPER JAVA SCRIPT (hjs) and ExTENDED JAVA SCRIPT (ej)
Like [Node.js](https://nodejs.org) are  the JavaScript runtimes built on Chrome's [V8](https://v8.dev/) JavaScript engine.  

- No strong asinhronous operations 
- easy
- all in one with some plugins (easy)
- easy make addons - plugins

**hjs**  and **ej** are mostly compatible, but...

**hjs** based on [QT4](https://www.qt.io/)  
**ej** on [uv](https://github.com/libuv/libuv)

***Binary Releases for Debian*** [bin](./bin)

Projects files on [Qt Creator](https://www.qt.io/)


#### API hjs
Look at [Bitches Brew](./bb.cpp  "bb.cpp ")  as [Miles](https://en.wikipedia.org/wiki/Miles_Davis) and find some...  
**hjs** have a graphical user interface [gui.cpp](./gui.cpp)  
linked with `-lcurl -lz -lv8 QT += core gui sql webkit`  
[fever test example](./bin/in.js)

#### API je

no many differences with **hjs** [look and see](./ej_proj/js_main.cpp "js_main.cpp")  
linked with `-lmemcached -lmemcachedutil -lz -luv -lv8 -lpq`   
[strongly hallo  example](./bin/in_ej.js "in_ej.js")

#### Plugins

[leveldb core](./plugins/leveldbjs/levw.cpp "levw.cpp") **->** [leveldb example](./bin/t_levw.js "t_levw.js")

[***License***](https://www.gnu.org/licenses/gpl.html "GPL")

[mail](mailto:hserg1965@rambler.ru "Send...")


