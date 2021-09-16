# cpp-json-lite
A simple json parser on C++

## C++ Standard
C++ 11 or later is required.

## Usage

#### Create 
```cpp
JSON json(std::string str); // it will jsonify the str.
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

#### Stringfy
```cpp
std::string to_string(std::string indent);
```

### About Author
HttoHu or 胡远韬 2021