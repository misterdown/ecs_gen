/*  main.cpp
    MIT License

    Copyright (c) 2024 Aidar Shigapov

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

*/

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
    DEFINITION_TYPE_MEMBER,
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
    "\tchar* data;\n"
    "\tsize_t dataSize;\n"
    "} component_info;\n"
    "static component_info componentsData[COMPONENT_COUNT][MAX_ENTITY_COUNT] = {};\n"
    "static id_t max_id = 0;\n"
    "static id_t freeIDs[MAX_ENTITY_COUNT] = {};\n"
    "static size_t freeIDCount = 0;\n";
}
string generate_c_after_components_definition(const vector<definition_info>& definitions) {
    string destroyComponentSector;
    string addComponentSector;
    string getComponentSector;
    bool firstCompDef = true;
    for (const auto& i : definitions) {
        if (i.componentID != INVALID_COMPONENT_INDEX) {
            const string componentIDStr = std::to_string(i.componentID);
            destroyComponentSector +=
            (firstCompDef ? string("\t\t\t") : string("\t\t\telse ")) + "if (i == " + componentIDStr +") {\n"
            "\t\t\t\t" + i.name + "_destroy((" + i.name + "*)componentsData[i][entity].data);\n"
            "\t\t\t}\n";
            firstCompDef = false;

            addComponentSector +=
            "void add_" + i.name + "(id_t entity) {\n"
            "\tcomponentsData[" + componentIDStr + "][entity].exist = 1;\n"
            "\tif (componentsData[" + componentIDStr + "][entity].data == 0) {\n"
            "\t\tcomponentsData[" + componentIDStr + "][entity].data = malloc(sizeof(" + i.name + "));\n"
            "\t\tcomponentsData[" + componentIDStr + "][entity].dataSize = sizeof(" + i.name + ");\n"
            "\t}\n"
            "\tfor (size_t i = 0u; i < sizeof(" + i.name + "); ++i)\n"
            "\t\tcomponentsData[" + componentIDStr + "][entity].data[i] = 0;\n"
            "}\n"
            "\n";

            getComponentSector +=
            i.name + "* get_" + i.name + "(id_t entity) {\n"
            "\tif (componentsData[" + componentIDStr + "][entity].exist == 0)\n"
            "\t\treturn 0;\n"
            "\treturn (" + i.name + "*)componentsData[" + componentIDStr + "][entity].data;\n"
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
    "void destroy_entity(id_t entity) {\n"
    "\tfor (size_t i = 0u; i < COMPONENT_COUNT; ++i) {\n"
    "\t\tif (componentsData[i][entity].exist) {\n"
    "\t\t\tcomponentsData[i][entity].exist = 0;\n"
    + destroyComponentSector +
    "\t\t}\n"
    "\t}\n"
    "\tfreeIDs[freeIDCount] = entity;\n"
    "\t++freeIDCount;\n"
    "}\n"
    "\n"
    "void cleanup() {\n"
    "\tfor (size_t i = 0u; i < COMPONENT_COUNT; ++i) {\n"
    "\t\tfor (size_t j = 0u; j < max_id; ++j) {\n"
    "\t\t\tif (componentsData[i][j].exist && componentsData[i][j].data != 0) {\n"
    "\t\t\t\tfree(componentsData[i][j].data);\n"
    "\t\t\t}\n"
    "\t\t}\n"
    "\t}\n"
    "}\n"
    "\n"
    + addComponentSector
    + getComponentSector;
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
            for (; (i < definitions.size()) && (definitions[i].type == DEFINITION_TYPE_MEMBER); ++i) {
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
string generate_c_foreach(const string& iteratorName, const vector<string>& components, const vector<definition_info>& definitions) {
    string checkSector;
    for (size_t ci = 0; ci < components.size(); ++ci) {
        const auto& c = components[ci];
        for (const auto& d : definitions) {
            if ((d.componentID != INVALID_COMPONENT_INDEX) && (c == d.name)) {
                const string strComponentID = std::to_string(d.componentID);
                checkSector += "!componentsData[" + strComponentID + "][" + iteratorName + "].exist" + (ci == (components.size() - 1) ? string("") : string(" || "));
            }
        }
    }

    return 
    "// foreach " + iteratorName + " [components] { your shitty(my) code }\n"
    "for (size_t " + iteratorName + " = 0u; " + iteratorName + " < max_id; ++" + iteratorName + ") {\n"
    "\tif (" + checkSector+ ")\n"
    "\t\tcontinue;\n"
    "\t\n"
    "\t//code\n"
    "}\n";
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
                definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_MEMBER, .typeSpec = i.value(), .name = "", .parentID = currentComponentID, .componentID = INVALID_COMPONENT_INDEX});

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
    std::cout << generate_c_foreach("entity", {"position", "velocity"}, definitions);
    std::cout << generate_c_destroy_entity("first", definitions);
    std::cout << generate_c_program_exit();
    std::cout << "}";
    return 0;
}
