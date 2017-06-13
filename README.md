## What is Comse?

Comse is a lightweigt common search engine,and it support fulltext search(index cross search and index sum search).

Main technology:

* dynamically increseaing or shrinking index data structures(for fast insert,delete index)
* use json data via http post to Interact(easy to use curl、python、php etc...)


## Building Comse

If First build:

```
git clone https://github.com/dodng/comse.git
cd comse/core
sh -x build.sh
```

else:

```
cd comse/core
make clean;make
```

## Running Comse

```
cd comse/service
sh -x run.sh
```

## How to use

1. load-all data(just load one time,default path: ./index/load.json.file)

2. Search data use search_ip and search_port,Other add,del,dump-all data use update_ip and update_port!

3. show_info field can storage and return a whatever json data which you want use.

python example:

* insert data
* delete data
* and search data
* or search data
* dump-all data(default path: ./index/dump.json.file)
* reload-all data(prevent index_num to max(int))

```
cd comse/examples ; python client_update.py message.add
cd comse/examples ; python client_update.py message.del
cd comse/examples ; python client_search.py message.and-search
cd comse/examples ; python client_search.py message.or-search
cd comse/examples ; python client_update.py message.dump-all
cd comse/examples ; python client_update.py message.reload-all

```

