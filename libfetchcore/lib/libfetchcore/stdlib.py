import module_register

def build(mod, relpath=".."):
    if not module_register.Register("stdlib"):
        return 

    mod.add_include("<vector>")

    std_module = mod.add_cpp_namespace("std")
    std_module.add_container('std::vector<int8_t>', 'uint8_t', 'vector', custom_name="StdVectorInt8") 
    std_module.add_container('std::vector<int16_t>', 'uint8_t', 'vector', custom_name="StdVectorInt16") 
    std_module.add_container('std::vector<int>', 'int', 'vector', custom_name="StdVectorInt32") 
    std_module.add_container('std::vector<int64_t>', 'uint8_t', 'vector', custom_name="StdVectorInt64") 
    std_module.add_container('std::vector<uint8_t>', 'uint8_t', 'vector', custom_name="StdVectorUInt8") 
    std_module.add_container('std::vector<uint16_t>', 'uint8_t', 'vector', custom_name="StdVectorUInt16") 
    std_module.add_container('std::vector<uint32_t>', 'uint32_t', 'vector', custom_name="StdVectorUInt32") 
    std_module.add_container('std::vector<uint64_t>', 'uint64_t', 'vector', custom_name="StdVectorUInt64") 
    std_module.add_container('std::vector<double>', 'double', 'vector', custom_name="StdVectorDouble") 
    std_module.add_container('std::vector<float>', 'float', 'vector', custom_name="StdVectorFloat") 
    std_module.add_container('std::vector<bool>', 'bool', 'vector', custom_name="StdVectorBool") 
