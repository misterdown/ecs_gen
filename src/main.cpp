#include <iostream>
#include <string>
#include <vector>
#include <sxt_head.hpp>

using std::string;
using std::vector;

constexpr size_t INVALID_PARENT_INDEX = ~0u;
constexpr size_t INVALID_COMPONENT_INDEX = ~0u;

enum definition_type {
    DEFINITION_TYPE_COMPONENT,
    DEFINITION_TYPE_BASE_TYPE,
    DEFINITION_TYPE_ID,
    DEFINITION_TYPE_FUNCTION,
    DEFINITION_TYPE_END,
};
struct definition_info {
    definition_type type;
    string typeSpec;
    string name;
    size_t parentID; // optional
    size_t componentID; // optional
};

template<class Iter, class ElseT>
Iter next_expected(Iter iter, sxt::token_type type, ElseT elseF) {
    if (iter->type() != type) {
        elseF();
        return iter;
    }
    return ++iter;
}

string generate_c_start_code(const vector<definition_info>& definitions) {
    size_t componentCount = 0;
    for (const auto& i : definitions) {
        if (i.componentID != INVALID_COMPONENT_INDEX)
            ++componentCount;
    }
    return
    "#include <malloc.h>\n"
    "#define COMPONENT_COUNT " + std::to_string(componentCount) + "\n"
    "#define MAX_ENTITY_COUNT 1024\n"
    "typedef size_t id_t;\ntypedef struct component_info {\n"
    "\tint exist;\n"
    "\tsize_t* data;\n"
    "\tsize_t dataSize;\n"
    "} component_info;\n"
    "typedef struct tiny_mask_vector {\n"
    "\tint owned[MAX_ENTITY_COUNT];\n"
    "\tsize_t ownedCount;\n"
    "} tiny_mask_vector;\n"
    "static component_info componentsData[COMPONENT_COUNT][MAX_ENTITY_COUNT] = {};\n"
    "static id_t max_id = 0;\n"
    "static id_t freeIDs[MAX_ENTITY_COUNT] = {};\n"
    "static size_t freeIDCount = 0;\n"
    "static tiny_mask_vector masks[COMPONENT_COUNT] = {};\n";
}
string generate_c_after_components_definition(const vector<definition_info>& definitions) {
    string destroyComponentSector;
    string addComponentSector;
    bool firstCompDef = true;
    for (const auto& i : definitions) {
        if (i.componentID != INVALID_COMPONENT_INDEX) {
            const string componentIDStr = std::to_string(i.componentID);
            destroyComponentSector +=
            (firstCompDef ? string("\t\t\t\t") : string("\t\t\t\telse ")) + "if (i == " + componentIDStr +") {\n"
            "\t\t\t\t\t" + i.name + "_destroy((" + i.name + "*)componentsData[i][entity].data);\n"
            "\t\t\t\t}\n";
            firstCompDef = false;

            addComponentSector +=
            "void add_" + i.name + "(id_t entity) {\n"
            "\tmasks[" + componentIDStr + "].owned[masks[" + componentIDStr + "].ownedCount] = entity;\n"
            "\t++masks[" + componentIDStr + "].ownedCount;\n"
            "\tconst size_t dataSize = ((sizeof(" + i.name + ") / sizeof(size_t)) + (sizeof(" + i.name + ") % sizeof(size_t) == 0 ? 0 : 1)) * sizeof(size_t);\n"
            "\tif (componentsData[" + componentIDStr + "][entity].data == 0) {\n"
            "\t\tcomponentsData[" + componentIDStr + "][entity].data = (size_t*)malloc(dataSize);\n"
            "\t\tcomponentsData[" + componentIDStr + "][entity].dataSize = dataSize;\n"
            "\t}\n"
            "\tfor (size_t i = 0u; i < dataSize / sizeof(size_t); ++i)\n"
            "\t\tcomponentsData[" + componentIDStr + "][entity].data[i] = 0;\n"
            "\tcomponentsData[" + componentIDStr + "][entity].exist = 1;\n"
            "}\n"
            "\n";
        }
    }


    return
    "id_t create() {\n"
    "\tif (freeIDCount == 0) {\n"
    "\t\treturn max_id++;\n"
    "\t} else {\n"
    "\t\t--freeIDCount;\n"
    "\t\treturn freeIDs[freeIDCount];\n"
    "\t}\n"
    "}\n"
    "\n"
    "void destroy_entity(const id_t entity) {\n"
    "\tfor (size_t i = 0u; i < COMPONENT_COUNT; ++i) {\n"
    "\t\tfor (size_t j = 0u; j < masks[i].ownedCount; ++j) {\n"
    "\t\t\tif (masks[i].owned[j] == entity) {\n"
    "\t\t\t\t--masks[i].ownedCount;\n"
    "\t\t\t\tmasks[i].owned[j] = masks[i].owned[masks[i].ownedCount];\n"
    "\t\t\t\tcomponentsData[i][entity].exist = 0;\n"
    + destroyComponentSector +
    "\t\t\t\tbreak;\n"
    "\t\t\t}\n"
    "\t\t}\n"
    "\t}\n"
    "\tfreeIDs[freeIDCount] = entity;\n"
    "\t++freeIDCount;\n"
    "}\n"
    "\n"
    "void cleanup() {\n"
    "\tfor (size_t i = 0u; i < COMPONENT_COUNT; ++i) {\n"
    "\t\tfor (size_t j = 0u; j < masks[i].ownedCount; ++j) {\n"
    "\t\t\tif (componentsData[i][j].data != 0) {\n"
    "\t\t\t\tfree(componentsData[i][j].data);\n"
    "\t\t\t}\n"
    "\t\t}\n"
    "\t}\n"
    "}\n"
    "\n"
    + addComponentSector;
}
string generate_c_destroy_some(const definition_info& definition) {
    if (definition.parentID == INVALID_PARENT_INDEX)
        return "";
    if ((definition.typeSpec == "float") || (definition.typeSpec == "int")) {
        return "(void)__w__->" + definition.name + ";\n";
    } else  {   
        return definition.typeSpec + "_destroy(&__w__->" + definition.name + ");\n";
    }
}
string generate_c_structures(const vector<definition_info>& definitions) {
    string result;
    size_t currentComponentIndex = INVALID_COMPONENT_INDEX;
    for (size_t i = 0; i < definitions.size(); ++i) {
        if (definitions[i].componentID != INVALID_COMPONENT_INDEX) {
            currentComponentIndex = i;
            const string& name = definitions[currentComponentIndex].name;
            result += "typedef struct " + name + " {\n";

            string destroyMembersSector;
            ++i;
            for (; (i < definitions.size()) && (definitions[i].componentID == INVALID_COMPONENT_INDEX) && (definitions[i].parentID != INVALID_PARENT_INDEX); ++i) {
                result += "\t" + definitions[i].typeSpec + " " + definitions[i].name + ";\n";
                destroyMembersSector += "\t" + generate_c_destroy_some(definitions[i]);
            }
            --i;
            
            result += 
            "} "  + name + ";\n"
            "void " + name + "_destroy(" + name + "* __w__) {\n"
            "\t(void)__w__;\n"
            + destroyMembersSector +
            "}\n";
        }
    }
    return result;
}
string generate_c_create_ent_with_name(const string& name) {
    return
    "// ent " + name + "\n"
    "const id_t " + name + " = create();\n";
}
string generate_c_create_add(const string& name, const string& componentName, const vector<definition_info>& definitions) {
    for (const auto& i : definitions) {
        if (i.componentID != INVALID_COMPONENT_INDEX && i.name == componentName) {
            const string strComponentID = std::to_string(i.componentID);
            return 
            "// add first " + componentName + ";\n"
            "add_" + componentName +"(" + name + ");\n";
        } 
    }
    exit(1);
    return "";
}
string generate_c_destroy_entity(const string& name, const vector<definition_info>& definitions) {
    return 
    "// destroy " + name + ";\n"
    "destroy_entity(" + name + ");\n";
}
string generate_c_program_exit() {
    return 
    "// program exit\n"
    "cleanup();\n";
}

void parse_definitions(const string& data, vector<definition_info>& definitions) {
    sxt::tokenizer<string> tokenizer(data.begin(), data.end());
    vector<sxt::value_token<string>> tokens;
    for (sxt::value_token<string> current = tokenizer.next_new_token(sxt::STX_EXT_TOKEN_TYPE_FLAG_BIT_NONE); current.is_valid(); current = tokenizer.next_new_token(sxt::STX_EXT_TOKEN_TYPE_FLAG_BIT_NONE)) {
        tokens.emplace_back(current);
    }

    size_t currentComponentID = INVALID_PARENT_INDEX;
    size_t componentCount = 0u;

    enum {
        EXPECTED_TYPE_DEFINITION_COMPONENT,
        EXPECTED_TYPE_COMPONENT_MEMBER_DEFINITION_TYPE,
    } expected_type = EXPECTED_TYPE_DEFINITION_COMPONENT;

    for (auto ii = tokens.begin(); ii != tokens.end(); ) {
        const auto& i = *ii;
        if (expected_type == EXPECTED_TYPE_DEFINITION_COMPONENT) {
            if (i.type() != sxt::STX_TOKEN_TYPE_WORD)
                exit(1);
            if (i.value() != "component")
                exit(1);

            ++ii;
            currentComponentID = definitions.size();
            definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_COMPONENT, .typeSpec = "", .name = ii->value(), .parentID = INVALID_PARENT_INDEX, .componentID = componentCount++});
            ii = next_expected(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});
            ii = next_expected(ii, sxt::STX_TOKEN_TYPE_LCURLY, [](){exit(1);});

            expected_type = EXPECTED_TYPE_COMPONENT_MEMBER_DEFINITION_TYPE;
            continue;

        } else if (expected_type == EXPECTED_TYPE_COMPONENT_MEMBER_DEFINITION_TYPE) {
            if (i.type() == sxt::STX_TOKEN_TYPE_WORD)  {
                definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_BASE_TYPE, .typeSpec = i.value(), .name = "", .parentID = currentComponentID, .componentID = INVALID_COMPONENT_INDEX});

                ++ii;
                definitions.back().name = ii->value();
                ii = next_expected(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});
                expected_type = EXPECTED_TYPE_COMPONENT_MEMBER_DEFINITION_TYPE;
                ii = next_expected(ii, sxt::STX_TOKEN_TYPE_SEMICOLON, [](){exit(1);});

                continue;
            } else if (i.type() == sxt::STX_TOKEN_TYPE_RCURLY) {
                expected_type = EXPECTED_TYPE_DEFINITION_COMPONENT;
                ++ii;
                ii = next_expected(ii, sxt::STX_TOKEN_TYPE_SEMICOLON, [](){exit(1);});

                continue;
            } else {
                exit(1);
            }
        }
        ++ii;
    }
    definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_END, .typeSpec = "", .name = "", .parentID = INVALID_PARENT_INDEX, .componentID = INVALID_COMPONENT_INDEX}); // eof
}

int main() {
    string data = "component point { float x; float y; }; component position { point vector; }; component velocity { point vector; };";
    /*
        component point { float x; float y; };
        component position { point vector; };
        component velocity { point vector; };

        fn main() {
            ent first
            add first position
            add first velocity
            destroy first
        }
    */
    sxt::tokenizer<string> tokenizer(data.begin(), data.end());
    vector<sxt::value_token<string>> tokens;
    for (sxt::value_token<string> current = tokenizer.next_new_token(sxt::STX_EXT_TOKEN_TYPE_FLAG_BIT_NONE); current.is_valid(); current = tokenizer.next_new_token(sxt::STX_EXT_TOKEN_TYPE_FLAG_BIT_NONE)) {
        tokens.emplace_back(current);
    }

    vector<definition_info> definitions;
    size_t currentComponentID = INVALID_PARENT_INDEX;
    size_t componentCount = 0u;

    parse_definitions(data, definitions);
    // for (const auto& i : definitions) {
    //     std::cout << i.name << " parent: " << (i.parentID == INVALID_PARENT_INDEX ? "null" : definitions[i.parentID].name) << '\n';
    // }
    std::cout << generate_c_start_code(definitions);
    std::cout << generate_c_structures(definitions);
    std::cout << generate_c_after_components_definition(definitions);
    std::cout << "int main() {\n";
    std::cout << generate_c_create_ent_with_name("first");
    std::cout << generate_c_create_add("first", "position", definitions);
    std::cout << generate_c_create_add("first", "velocity", definitions);
    std::cout << generate_c_destroy_entity("first", definitions);
    std::cout << generate_c_program_exit();
    std::cout << "}";
    return 0;
}