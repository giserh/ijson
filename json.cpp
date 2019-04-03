
#include "json.h"

int JsonParser::parse_object(ISlice buf) {
    if(buf.size() < 10) throw Exception(Exception::DATA_ERROR);
    const char *ptr = buf.ptr();
    if(ptr[0] != '{') throw Exception(Exception::DATA_ERROR);
    this->buf = buf;

    index = 1;
    bool skip_body = false;

    Slice key, value;
    this->method.clear();
    this->id.clear();
    this->params.clear();
    this->name.clear();

    for(;index < buf.size();) {
        strip();
        key = read_string();
        strip();
        if(next() != ':') throw Exception(Exception::DATA_ERROR);
        strip();
        char a = peek();
        if(a == '"') value = read_string();
        else if(a == '{' || a == '[') {
            // inner object
            int lvl = 0;
            int start = index;
            for(;index<buf.size();) {
                a = ptr[index];
                if(a == '{' || a == '[') {
                    lvl++;
                } else if(a == '}' || a == ']') {
                    lvl--;
                    if(lvl==0) {
                        index++;
                        break;
                    }
                } else if(a == '"') {
                    read_string();
                    continue;
                }
                index++;
            }
            value = Slice(&ptr[start], index - start);
        } else value = read_object();

        //std::cout << key.as_string() << " = " << value.as_string() << std::endl;
        if(key.equal("method")) {
            this->method = value;
            if(!value.starts_with("/rpc/")) {
                skip_body = true;
                if(this->id.valid()) return 1;
            }
        } else if(key.equal("id")) {
            this->id = value;
            if(skip_body) return 1;
        } else if(key.equal("params")) this->params = value;
        else if(key.equal("name")) this->name = value;

        strip();
        a = next();
        if(a == ',') continue;
        if(a == '}') break;
        throw Exception(Exception::DATA_ERROR);
    }

    return 0;
}

void JsonParser::strip() {
    while(index < buf.size()) {
        char a = buf.ptr()[index];
        if(a == ' ' || a == '\n' || a == '\r' || a == '\t') {
            index++;
            continue;
        }
        return;
    }
    throw Exception(Exception::DATA_ERROR);
}

Slice JsonParser::read_object() {
    int start = index;
    char a;
    for(;a = next();) {
        if(a == ' ' || a == ',' || a == '}' || a == '\n' || a == '\r') break;
    }
    index--;
    return Slice(&buf.ptr()[start], index - start);
}

Slice JsonParser::read_string() {
    if(next() != '"') throw Exception(Exception::DATA_ERROR);
    int start = index;
    const char *ptr = buf.ptr();
    for(;index < buf.size();index++) {
        if(ptr[index] == '"' && ptr[index-1] != '\\') {
            index++;
            return Slice(&ptr[start], index - start - 1);
        }
    }
    throw Exception(Exception::DATA_ERROR);
}