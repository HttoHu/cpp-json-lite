# cpp-json-lite
A simple json parser on C++ 

supports UTF-8 encoding

`bash_test.sh` to build test_cases, the unittest only contains regressions test

## C++ Standard
C++ 11 or later is required.

## Usage

**Be careful when copying a JSON object, because when you copy A to B, A should no longer be used (like be moved, TODO: using move semantics, disable copy construction and assignment)**
```cpp
JSON A=B; 
A.add_pair("GOOD",B); // BOOM!!!! The node object inside has been deleted twice
```cpp
A.add_pair("GOOD",B.clone()); // Okay
```
```cpp
// and never add itself
A.add_pair("Self",A); // BOOM
A.add_pair("Self",A.clone()); // Okay
```

#### Create or Clone

```cpp
JSON json(std::string str); // it will jsonify the str.
JSON::clone(); // clone a json object
```

#### Visit

* get int value by JSON::get_int();
```cpp
int val=json.get_int();
```
* get string value by JSON::get_str();
  
```cpp
std::string str=json.get_str();
```
* visit map by string index
```cpp
JSON sub_json=json["servers"]["port"];
int port=sub_json.get_int();
```
* visit array by number index
```cpp
int val=json[0].get_int();
```
* get list
```cpp
std::vector<JSON> JSON::get_list() const;
```
* get map
```cpp
std::map<std::string, JSON> JSON::get_map() const;
```

#### Get elements count
```cpp
// for map
size_t count() const;
// for list
size_t length() const;
```

#### to_string
```cpp
std::string to_string(std::string indent);
```

#### Add elements
```cpp
// for map
void add_pair(const std::string &str, JSON);
// for array
void push(JSON);
```
e.g.
```cpp
    JSON json("{}");

    json.add_pair("port", JSON::val(8080));
    json.add_pair("server", JSON::val("localhost"));

    JSON arr_json("[]");
    arr_json.push(JSON::val("GOOD Boy"));
    arr_json.push(JSON::val(123));
    arr_json.push(json.clone());
```

#### Build json by value
```cpp
static JSON val(int val);
static JSON val(const std::string &str);
static JSON map(const std::map<std::string, JSON> &table);
static JSON array(const std::vector<JSON> &vec);
```
example 
```cpp
    JSON json = JSON::map({
        {"port",JSON::val(123)},
        {"path",JSON::array({
            JSON::val(1),
            JSON::val("Hello World")
        })}
    });
```




#### Extend grammar
If you want to append some raw data (like pictures or some binary files), you can build your json as follows.
(raw-length)$raw_contenst$

```json
{
    "picture":(512)$the binary content of the picture$
}
```
here are some methods;

1. create raw object
```cpp
static JSON raw(const std::vector<unsigned char> &vec);
static JSON raw(std::vector<unsigned char> &&vec);
```
2. get raw object 
```cpp
std::vector<unsigned char> &get_raw() const;
```

3. if you want to output the json without raw content you can call view(), view is similar to to_string, but it replaces the raw data with its length.
```cpp
std::string view(std::string indent = "    ") const;
```

### About the author
Htto Hu or 胡远韬 2021