#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include "include/json.hpp"
using json = nlohmann::json;

namespace mkh_avro2 {

const std::vector<std::string> scalars{"int", "long", "double", "float", "boolean", "string"};
bool is_scalar(std::string ttype){
    return std::find(scalars.begin(), scalars.end(), ttype) != scalars.end();
}
int get_scalar_type(std::string ntype){
    auto it = find(scalars.begin(), scalars.end(), ntype);
    if (it != scalars.end()){
        int index = it - scalars.begin();
        return index;
    }else{
        return -1;
    } 
}

typedef struct SchemaItem {
    int obj_type = -1;
    int scalar_type = -1;
    int obj_simple_type = 0;
    int is_nullable = 0;
    std::string obj_name;
    std::vector<SchemaItem *> childItems;
    std::string obj_field = "complex";

    void set_undefined_obj_type(int ot){
        if(obj_type == -1){
            obj_type = ot;
        }
    }

    SchemaItem * read_object_type(json otype){
        SchemaItem * intsi;
        if(otype["type"] == "array"){
            intsi = new SchemaItem("array", otype["items"], 2);
        }else if(otype["type"] == "map"){
            intsi = new SchemaItem("map", otype["values"], 4);
        }else if(otype["type"] == "record"){
            intsi = new SchemaItem("record", otype["fields"], 3);
        }else{
            if(otype["type"].is_string() && is_scalar(otype["type"])){
                // record field with scalar type
                intsi = new SchemaItem(otype["name"], otype["type"], 0);
            }else if(otype["type"].is_object() && otype["type"].contains("type") && otype["type"]["type"] == "array"){ 
            // array of simple types
                intsi = new SchemaItem(otype["name"], otype["type"]["items"], 2);
            }else if(otype["type"].is_object() && otype["type"].contains("type") && otype["type"]["type"] == "map"){
                intsi = new SchemaItem(otype["name"], otype["type"]["values"], 4);
            }else if(otype["type"].is_object() && otype["type"].contains("type") && otype["type"]["type"] == "record"){
                intsi = new SchemaItem(otype["name"], otype["type"]["fields"], 3);
            }else{
                    intsi = new SchemaItem(otype["name"], otype["type"]);
            }
        }
        return intsi;
    }

    std::vector<SchemaItem *> read_internal_types(json utypes){
        std::vector<SchemaItem *> retv;
        SchemaItem * intsi;
        if(utypes.is_array()){
            for (auto it : utypes){
                if(it.is_string() && "null" == it){
                    std::cout << "Nullable!!!" << '\n' << '\r';
                    is_nullable = 1;
                }else if(it.is_string()){ 
                    std::cout << "simple type!!! -> " << it <<'\n' << '\r';
                    obj_field = it;
                    intsi = new SchemaItem("union_member", it, 0);
                    retv.push_back(intsi);
                }else if(it.is_object()){
                    retv.push_back(read_object_type(it));
                }else{
                    throw std::runtime_error("Not implemented 5");
                }
            }
        }else if(utypes.is_object()){
            retv.push_back(read_object_type(utypes));
        }else{
            throw std::runtime_error("Not implemented 7");
        }
        return retv;
    }

    SchemaItem(std::string name, json jtypes, int ot){
        obj_name = name;
        obj_type = ot;
        std::cout << "SI constructor 3args" << '\n' << '\r';
        if(jtypes.is_string()){
            // scalar types
            set_undefined_obj_type(0);
            std::cout << "\t" << "SI constructor simlpe type " << jtypes << '\n' << '\r';
            obj_field = jtypes;
            scalar_type = get_scalar_type(jtypes);
        }else{
            childItems = read_internal_types(jtypes);
        }
    }
    SchemaItem(std::string name, json jtypes){
        obj_name = name;
        std::cout << "SI constructor 2args" << '\n' << '\r';
        if(jtypes.is_string()){
            // scalar types
            set_undefined_obj_type(0);
            std::cout << "\t" << "SI constructor simlpe type " << jtypes << '\n' << '\r';
            obj_field = jtypes;
            if("map" == jtypes){
                obj_type = 4;
            }else if("array" == jtypes){
                obj_type = 2;
            }else if("record" == jtypes){
                obj_type = 3;
            }
        }else if(jtypes.is_array()){ // union type
            obj_type = 1;
            childItems = read_internal_types(jtypes);
        }else{
            childItems = read_internal_types(jtypes);
        }
    }
} SchemaItem;


SchemaItem*  read_schema(std::string);
ERL_NIF_TERM encode(ErlNifEnv*, SchemaItem*, const ERL_NIF_TERM*);
int encodevalue(SchemaItem*, ErlNifEnv*, ERL_NIF_TERM*, std::vector<uint8_t>*);
int encodescalar(int, ErlNifEnv*, ERL_NIF_TERM*, std::vector<uint8_t>*); 
int encodeunion(SchemaItem*, ErlNifEnv*, ERL_NIF_TERM*, std::vector<uint8_t>*); 
int encodearray(SchemaItem*, ErlNifEnv*, ERL_NIF_TERM*, std::vector<uint8_t>*);
int encode_int(ErlNifEnv*, ERL_NIF_TERM*, std::vector<uint8_t>*);
int encode_long(ErlNifEnv*, ERL_NIF_TERM*, std::vector<uint8_t>*);
int encode_long_fast(ErlNifEnv*, int64_t, std::vector<uint8_t>*);
int encode_float(ErlNifEnv*, ERL_NIF_TERM*, std::vector<uint8_t>*);
int encode_double(ErlNifEnv*, ERL_NIF_TERM*, std::vector<uint8_t>*);
int encode_string(ErlNifEnv*, ERL_NIF_TERM*, std::vector<uint8_t>*);
int encode_boolean(ErlNifEnv*, ERL_NIF_TERM*, std::vector<uint8_t>*);
int enif_get_bool(ErlNifEnv*, ERL_NIF_TERM*, bool*);
size_t encodeInt32(int32_t, std::array<uint8_t, 5> &output) noexcept;

ERL_NIF_TERM encode(ErlNifEnv* env, SchemaItem* si, const ERL_NIF_TERM* input){
    ERL_NIF_TERM binary;
    ERL_NIF_TERM key;
    ERL_NIF_TERM val;
    ErlNifBinary bin;
    ErlNifBinary retbin;
    int len;
    std::vector<uint8_t> retv;

    retv.reserve(10000);

    std::cout << "ENC0\n\r";
    if(!enif_is_map(env, *input)){
    	return enif_make_badarg(env);
    }
    
    std::cout << "ENC1 \n\r";

    for( auto it: si->childItems ){
        len = it->obj_name.size();
        std::cout << "E erlav encode field: " << it->obj_name <<'\n' << '\r';
        enif_alloc_binary(len, &bin);
        const auto *p = reinterpret_cast<const uint8_t *>(it->obj_name.c_str());
        memcpy(bin.data, p, len);
        key = enif_make_binary(env, &bin);

        if(enif_get_map_value(env, *input, key, &val)){
            int encodeCode = encodevalue(it, env, &val, &retv);
            if(encodeCode != 0){
                throw encodeCode;
            }
        }else if(it->is_nullable == 1){
            std::cout << "No value for field: " << key << '\n' << '\r';
            retv.push_back(0);
        }

    }

    auto retlen = retv.size();
    enif_alloc_binary(retlen, &retbin);
    memcpy(retbin.data, retv.data(), retlen);
    binary = enif_make_binary(env, &retbin);
    return binary;
}


int encodevalue(SchemaItem* si, ErlNifEnv* env, ERL_NIF_TERM* val, std::vector<uint8_t>* ret){ 
    switch(si->obj_type){
        case 0:
            std::cout << "ENCODE scalar VALUE!!!" << si->scalar_type << "\n\r";
            return encodescalar(si->scalar_type, env, val, ret);
        case 1:
            return encodeunion(si, env, val, ret);
        case 2:
            return encodearray(si, env, val, ret);
        default:
            std::cout << "ENCODE VALUE!!!\n\r";

    }
    return 0;
}

int encodearray(SchemaItem* si, ErlNifEnv* env, ERL_NIF_TERM* val, std::vector<uint8_t>* ret){ 
    std::cout << "ENCODE ARRAY!!!\n\r";
    retunt 0;
}

int encodeunion(SchemaItem* si, ErlNifEnv* env, ERL_NIF_TERM* val, std::vector<uint8_t>* ret){ 
    std::array<uint8_t, 5> output;
    //assume null in union always first item
    if(si->childItems.size() == 1){
        std::cout << "ENCODE SIMPLE UNION!!!: " << si->is_nullable <<  "\n\r";
        ret->push_back(1 + si->is_nullable); 
        return encodevalue(si->childItems[0], env, val, ret);
    }else{
        std::cout << "ENCODE UNION!!!\n\r";
        ret->push_back(0); // reserve first for type index
        auto union_index = ret->size() - 1;
        for (auto iter = si->childItems.begin(); iter != si->childItems.end(); ++iter) {
            int index = std::distance(si->childItems.begin(), iter);
            auto ret_code = encodevalue(*iter, env, val, ret);
            if(ret_code == 0){
                encodeInt32(index + si->is_nullable, output);
                ret->at(union_index) = output[0];
                return ret_code;
            }
        }
        if(si->is_nullable == 1){
            return 0;
        }
        return 7;
    }
    return 0;
}

int encodescalar(int scalar_type, ErlNifEnv* env, ERL_NIF_TERM* val, std::vector<uint8_t>* ret){ 
    switch(scalar_type){
        case 0:
            return encode_int(env, val, ret);
        case 1:
            return encode_long(env, val, ret);
        case 2:
            return encode_double(env, val, ret);
        case 3:
            return encode_float(env, val, ret);
        case 4:
            return encode_boolean(env, val, ret);
        case 5:
            return encode_string(env, val, ret);
    }
    return 1;
}

SchemaItem*  read_schema(std::string schemaName){
    SchemaItem* si;
    std::ifstream f(schemaName);
    json data = json::parse(f);

    si = new SchemaItem(data["name"], data["fields"], 3);
    return si;
}


// ------ primitive encoders

uint64_t encodeZigzag64(int64_t input) noexcept {
    // cppcheck-suppress shiftTooManyBitsSigned
    return ((input << 1) ^ (input >> 63));
}

int64_t decodeZigzag64(uint64_t input) noexcept {
    return static_cast<int64_t>(((input >> 1) ^ -(static_cast<int64_t>(input) & 1)));
}

uint32_t encodeZigzag32(int32_t input) noexcept {
    // cppcheck-suppress shiftTooManyBitsSigned
    return ((input << 1) ^ (input >> 31));
}

int32_t decodeZigzag32(uint32_t input) noexcept {
    return static_cast<int32_t>(((input >> 1) ^ -(static_cast<int64_t>(input) & 1)));
}

size_t
encodeInt64(int64_t input, std::array<uint8_t, 10> &output) noexcept {
    auto val = encodeZigzag64(input);

    // put values in an array of bytes with variable length encoding
    const int mask = 0x7F;
    auto v = val & mask;
    size_t bytesOut = 0;
    while (val >>= 7) {
        output[bytesOut++] = (v | 0x80);
        v = val & mask;
    }

    output[bytesOut++] = v;
    return bytesOut;
}

size_t
encodeInt32(int32_t input, std::array<uint8_t, 5> &output) noexcept {
    auto val = encodeZigzag32(input);

    // put values in an array of bytes with variable length encoding
    const int mask = 0x7F;
    auto v = val & mask;
    size_t bytesOut = 0;
    while (val >>= 7) {
        output[bytesOut++] = (v | 0x80);
        v = val & mask;
    }

    output[bytesOut++] = v;
    return bytesOut;
}

int encode_int(ErlNifEnv* env, ERL_NIF_TERM* input, std::vector<uint8_t>* ret){
    std::array<uint8_t, 5> output;
    int32_t i32;
    
    if (!enif_get_int(env, *input, &i32)) {
        return 1;
    }
    auto len = encodeInt32(i32, output);
    ret->insert(ret->end(), output.data(), output.data() + len);
    return 0;
}

int encode_long(ErlNifEnv* env, ERL_NIF_TERM* input, std::vector<uint8_t>* ret){
    std::array<uint8_t, 10> output;
    int64_t i64;
    
    if (!enif_get_int64(env, *input, &i64)) {
        return 2;
    }
    auto len = encodeInt64(i64, output);
    ret->insert(ret->end(), output.data(), output.data() + len);
    return 0;
}

int encode_long_fast(ErlNifEnv* env, int64_t input, std::vector<uint8_t>* ret){
    std::array<uint8_t, 10> output;
    auto len = encodeInt64(input, output);
    ret->insert(ret->end(), output.data(), output.data() + len);
    return 0;
}

int encode_float(ErlNifEnv* env, ERL_NIF_TERM* input, std::vector<uint8_t>* ret){
    float f;
    double dbl;
    
    if (!enif_get_double(env, *input, &dbl)) {
        return 3;
    }

    f = dbl;
    auto len = sizeof(float);
    const auto *p = reinterpret_cast<const uint8_t *>(&f);

    ret->insert(ret->end(), p, p + len);
    return 0;

}

int encode_double(ErlNifEnv* env, ERL_NIF_TERM* input, std::vector<uint8_t>* ret){
    double dbl;
    
    if (!enif_get_double(env, *input, &dbl)) {
        return 4;
    }

    auto len = sizeof(double);
    const auto *p = reinterpret_cast<const uint8_t *>(&dbl);
    ret->insert(ret->end(), p, p + len);
    return 0;
}

int encode_string(ErlNifEnv* env, ERL_NIF_TERM* input, std::vector<uint8_t>* ret){
    std::array<uint8_t, 10> output;
    ErlNifBinary sbin;

    if (!enif_inspect_binary(env, *input, &sbin)) {
        return 5;
    }

    auto len = sbin.size;
    auto len2 = encodeInt64(len, output);
    ret->insert(ret->end(), output.data(), output.data() + len2);
    ret->insert(ret->end(), sbin.data, sbin.data + len);

    return 0;
}

int encode_boolean(ErlNifEnv* env, ERL_NIF_TERM* input, std::vector<uint8_t>* ret){
    bool bl;
    
    if (!enif_get_bool(env, input, &bl)) {
        return 6;
    }

    ret->push_back(bl ? 1: 0);
    return 0;
}

inline int enif_get_bool(ErlNifEnv* env, ERL_NIF_TERM* term, bool* cond)
{
    char atom[256];
    int  len;
    if ((len = enif_get_atom(env, *term, atom, sizeof(atom), ERL_NIF_LATIN1)) == 0) {
        return false;
    }
    
    *cond = (std::strcmp(atom, "false") != 0 && std::strcmp(atom, "nil") != 0);
    
    return true;
}
}