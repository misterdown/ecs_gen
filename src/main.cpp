/*
    main.cpp
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
#include <algorithm>
#include <sxt_head.hpp>

using std::string;
using std::vector;
using std::cout;
using std::to_string;

// useful only in C++11
template <class IterT, class FindT, class PredT>
IterT find_pred(IterT begin, const IterT end, const FindT& value, PredT pred) {
    for (;begin != end; ++begin) {
        if (pred(*begin, value))
            return begin;
    }
    return end;
}

enum definition_type {
    DEFINITION_TYPE_STRUCT,         // opcode [ NAME ]
    DEFINITION_TYPE_COMPONENT,      // opcode [ NAME COMPONENT_ID ]
    DEFINITION_TYPE_MEMBER,         // opcode [ TYPENAME NAME ]
    DEFINITION_TYPE_FUNCTION,       // opcode [ RETURN_TYPENAME NAME ARGS... ]
    DEFINITION_TYPE_CREATE,         // opcode [ NAME ]
    DEFINITION_TYPE_ADD_COMPONENTS, // opcode [ NAME COMPONENTS... ]
    DEFINITION_TYPE_DESTROY_ENTITY, // opcode [ NAME ]
    DEFINITION_TYPE_FOREACH_CYCLE,  // opcode [ ITERATOR_NAME COMPONENTS... ]
    DEFINITION_TYPE_BODY_BEGIN,     // opcode [ ]
    DEFINITION_TYPE_BODY_END,       // opcode [ ]
    DEFINITION_TYPE_EOF,
};
const char* definition_type_to_string(definition_type deft) {
    switch (deft) {
        case DEFINITION_TYPE_STRUCT: return         "STRUCT";
        case DEFINITION_TYPE_COMPONENT: return      "COMPONENT";
        case DEFINITION_TYPE_MEMBER: return         "MEMBER";
        case DEFINITION_TYPE_FUNCTION: return       "FUNCTION";
        case DEFINITION_TYPE_CREATE: return         "CREATE";
        case DEFINITION_TYPE_ADD_COMPONENTS: return "ADD_COMPONENTS";
        case DEFINITION_TYPE_DESTROY_ENTITY: return "DESTROY_ENTITY";
        case DEFINITION_TYPE_FOREACH_CYCLE: return  "FOREACH";
        case DEFINITION_TYPE_BODY_BEGIN: return     "BODY_BEGIN";
        case DEFINITION_TYPE_BODY_END: return       "BODY_END";
        case DEFINITION_TYPE_EOF: return            "PROGRAM_END";
        default: return "";
    }
}

struct definition_info {
    definition_type type;
    vector<string> opcode;
};
struct variable_info {
    string typeName;
    string name;
};

#define ERROR_REPORT(msg__) do { \
    cout << (to_string(ii->line()) + ":" + to_string(ii->column()) + ": " + (msg__)); \
    exit(1); \
} while(false)

template<class Iter, class ElseT>
Iter predict_next(Iter iter, sxt::token_type type, ElseT elseF) {
    ++iter;
    if (iter->type() != type)
        elseF();
    return iter;
}

string generate_c_start_code(const vector<definition_info>& definitions) {
    size_t componentCount = 0;
    for (const auto& i : definitions) {
        if (i.type == DEFINITION_TYPE_COMPONENT)
            ++componentCount;
    }
    return
    "#include <malloc.h>\n"
    "#define COMPONENT_COUNT " + to_string(componentCount) + "\n"
    "#define MAX_ENTITY_COUNT 1024\n"
    "typedef size_t entity_t;\n"
    "typedef struct component_info {\n"
    "\tint exist;\n"
    "\tchar* data;\n"
    "\tsize_t dataSize;\n"
    "} component_info;\n"
    "static component_info componentsData[COMPONENT_COUNT][MAX_ENTITY_COUNT] = {};\n"
    "static int existMask[MAX_ENTITY_COUNT] = {};\n"
    "static entity_t max_id = 0;\n"
    "static entity_t freeIDs[MAX_ENTITY_COUNT] = {};\n"
    "static size_t freeIDCount = 0;\n";
}

string generate_c_after_components_definition(const vector<definition_info>& definitions) {
    string destroyComponentSector;
    string addComponentSector;
    string getComponentSector;
    bool firstCompDef = true;
    for (const auto& i : definitions) {
        if (i.type == DEFINITION_TYPE_COMPONENT) {
            const auto& name = i.opcode.at(0);
            const auto& componentIDStr = i.opcode.at(1);
            destroyComponentSector +=
            (firstCompDef ? string("\t\t\t") : string("\t\t\telse ")) + "if (i == " + componentIDStr +") {\n"
            "\t\t\t\t" + name + "_destroy((" + name + "*)componentsData[i][entity].data);\n"
            "\t\t\t}\n";
            firstCompDef = false;

            addComponentSector +=
            "void add_" + name+ "(entity_t entity) {\n"
            "\tcomponentsData[" + componentIDStr + "][entity].exist = 1;\n"
            "\texistMask[entity] = 1;\n"
            "\tif (componentsData[" + componentIDStr + "][entity].data == 0) {\n"
            "\t\tcomponentsData[" + componentIDStr + "][entity].data = malloc(sizeof(" + name + "));\n"
            "\t\tcomponentsData[" + componentIDStr + "][entity].dataSize = sizeof(" + name + ");\n"
            "\t}\n"
            "\tfor (size_t i = 0u; i < sizeof(" + name + "); ++i)\n"
            "\t\tcomponentsData[" + componentIDStr + "][entity].data[i] = 0;\n"
            "}\n"
            "\n";

            getComponentSector +=
            name + "* get_" + name + "(entity_t entity) {\n"
            "\tif (componentsData[" + componentIDStr + "][entity].exist == 0)\n"
            "\t\treturn 0;\n"
            "\treturn (" + name + "*)componentsData[" + componentIDStr + "][entity].data;\n"
            "}\n"
            "\n";
        }
    }

    return
    "entity_t create() {\n"
    "\tif (freeIDCount == 0) {\n"
    "\t\treturn max_id++;\n"
    "\t} else {\n"
    "\t\t--freeIDCount;\n"
    "\t\treturn freeIDs[freeIDCount];\n"
    "\t}\n"
    "}\n"
    "\n"
    "void destroy_entity(entity_t entity) {\n"
    "\texistMask[entity] = 0;\n"
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
    const auto& typeName = definition.opcode.at(0);
    const auto& name = definition.opcode.at(1);

    if (definition.type == DEFINITION_TYPE_MEMBER) {
        if ((typeName == "float") || (typeName == "int")) {
            return "(void)__w__->" + name + ";\n";
        } else  {
            return typeName + "_destroy(&__w__->" + name + ");\n";
        }
    } else {
        cout << "invalid definition\n";
        exit(1);
    }
}

string generate_c_structures(const vector<definition_info>& definitions) {
    string result;
    for (size_t i = 0; i < definitions.size(); ++i) {
        const definition_type definitionType = definitions[i].type;
        if ((definitionType == DEFINITION_TYPE_COMPONENT) || (definitionType == DEFINITION_TYPE_STRUCT)) {
            const auto& name = definitions[i].opcode.at(0);
            result += "typedef struct " + name + " {\n";

            string destroyMembersSector;
            ++i;
            for (; (i < definitions.size()) && (definitionType == DEFINITION_TYPE_MEMBER); ++i) {
                const auto& typeName = definitions[i].opcode.at(0);
                const auto& memberName = definitions[i].opcode.at(1);
                result += "\t" + typeName + " " + memberName + ";\n";
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
    "const entity_t " + name + " = create();\n";
}

string generate_c_add_coponents(const definition_info& addDefinition, const vector<definition_info>& definitions) {
    string result;
    const auto& entityName = addDefinition.opcode.at(0);
    
    for (size_t j = 1; j < addDefinition.opcode.size(); ++j) {
        const auto& componentName = addDefinition.opcode[j];
        bool found = false;

        for (const auto& i : definitions) {
            if ((i.type == DEFINITION_TYPE_COMPONENT) && (i.opcode.at(0) == componentName)) {
                found = true;
                const auto& name = i.opcode.at(0);
                const auto& strComponentID = i.opcode.at(1);
                result +=
                "// add first " + componentName + "\n"
                "add_" + componentName +"(" + entityName + ");\n";
                break;
            }
        }
        if (!found) {
            cout << "component not found\n";
            exit(1);
        }
    }

    
    return result;
}

string generate_c_destroy_entity(const string& name) {
    return
    "// destroy " + name + "\n"
    "destroy_entity(" + name + ");\n";
}

string generate_c_program_exit() {
    return
    "// program exit\n"
    "cleanup();\n";
}

string generate_c_foreach(const definition_info& foreachDefinition, const vector<definition_info>& definitions) {
    const auto& iteratorName = foreachDefinition.opcode.at(0);
    if (foreachDefinition.opcode.size() < 2) {
        return
        "// foreach " + iteratorName + " [components] { your shitty(my) code }\n"
        "for (entity_t " + iteratorName + " = 0u; " + iteratorName + " < max_id; ++" + iteratorName + ")\n"
        "\tif (existMask[" + iteratorName + "]) ";
    }
    string checkSector;
    for (size_t ci = 1; ci < foreachDefinition.opcode.size(); ++ci) {
        const auto& component = foreachDefinition.opcode[ci];

        for (const auto& d : definitions) {
            if ((d.type == DEFINITION_TYPE_COMPONENT) && (component == d.opcode.at(0))) {
                const string strComponentID = d.opcode.at(1);
                checkSector += "componentsData[" + strComponentID + "][" + iteratorName + "].exist" + (ci == (foreachDefinition.opcode.size() - 1) ? string("") : string(" && "));
                break;
            }
        }
    }

    return
    "// foreach " + iteratorName + " [components] { your shitty(my) code }\n"
    "for (entity_t " + iteratorName + " = 0u; " + iteratorName + " < max_id; ++" + iteratorName + ")\n"
    "\tif (" + checkSector + ") ";
}

template<class IterT>
IterT parse_function(IterT begin, IterT end, vector<variable_info>& variableContext, vector<definition_info>& definitions) {
    auto ii = begin;
    for (; (ii != end) && (ii->type() != sxt::STX_TOKEN_TYPE_RCURLY) && (ii->type() != sxt::STX_TOKEN_TYPE_SEMICOLON); ++ii) {
        if (ii->type() == sxt::STX_TOKEN_TYPE_WORD) {
            if (ii->value() == "ent") {
                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});

                const auto& name = ii->value();
                definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_CREATE, .opcode = { name }});
                variableContext.emplace_back(variable_info{.typeName = "ent", .name = name});

                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_SEMICOLON, [](){exit(1);});
            } else if (ii->value() == "foreach") {
                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});

                const auto& iteratorName = ii->value();
                definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_FOREACH_CYCLE, .opcode = { iteratorName }});

                definition_info& foreachDefinition = definitions.back();
                variableContext.emplace_back(variable_info{.typeName = "ent", .name = iteratorName});

                ++ii;
                for (; (ii != end) && (ii->type() != sxt::STX_TOKEN_TYPE_LCURLY); ++ii)
                    foreachDefinition.opcode.emplace_back(ii->value());
                ++ii;

                definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_BODY_BEGIN, .opcode = { }});
                ii = parse_function(ii, end, variableContext, definitions);
                definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_BODY_END, .opcode = { }});

            } else {
                const auto maybeVariable = find_pred(variableContext.begin(), variableContext.end(), ii->value(),
                    [](const variable_info& info1, const string& name) {
                        return info1.name == name;
                    });

                if (maybeVariable == variableContext.end())
                    ERROR_REPORT("unknown variable name: " + ii->value() + "\n");

                const variable_info& variable = *maybeVariable;
                if (variable.typeName == "ent") {
                    ii = predict_next(ii, sxt::STX_TOKEN_TYPE_DOT, [](){exit(1);});
                    ii = predict_next(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});
                    const auto& methodName = *ii;
                    if (methodName.value() == "add") {
                        ii = predict_next(ii, sxt::STX_TOKEN_TYPE_LESS, [](){exit(1);});
                        ii = predict_next(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});

                        definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_ADD_COMPONENTS, .opcode = { variable.name }});
                        definition_info& addComponentDefinition = definitions.back();

                        for (;; ++ii) {
                            if (ii == end)
                                ERROR_REPORT("EOF while parsing 'add' method\n");
                            addComponentDefinition.opcode.emplace_back(ii->value());
                            ++ii;

                            if (ii->type() == sxt::STX_TOKEN_TYPE_MORE) {
                                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_LPAREN, [](){exit(1);});
                                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_RPAREN, [](){exit(1);});
                                break;
                            } else if (ii->type() == sxt::STX_TOKEN_TYPE_COMMA) {
                                continue;
                            } else {
                                ERROR_REPORT("invalid add components syntax\n");
                            }
                        }
                    } else if (methodName.value() == "destroy") {
                        ii = predict_next(ii, sxt::STX_TOKEN_TYPE_LPAREN, [](){exit(1);});
                        ii = predict_next(ii, sxt::STX_TOKEN_TYPE_RPAREN, [](){exit(1);});
                        definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_DESTROY_ENTITY, .opcode = { variable.name }});
                    }
                    ii = predict_next(ii, sxt::STX_TOKEN_TYPE_SEMICOLON, [](){exit(1);});
                }
            }
        } else if (ii->type() == sxt::STX_TOKEN_TYPE_LCURLY) {
            definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_BODY_BEGIN, .opcode = { }});
        } else {
            ERROR_REPORT("invalid token type\n");
        }
    }
    return ii;
}

void parse_definitions(const string& data, vector<definition_info>& definitions) {
    sxt::tokenizer<string> tokenizer(data.begin(), data.end());
    vector<sxt::position_token<string>> tokens;
    for (sxt::position_token<string> current = tokenizer.next_position_token(sxt::STX_EXT_TOKEN_TYPE_FLAG_BIT_NONE); current.is_valid(); current = tokenizer.next_position_token(sxt::STX_EXT_TOKEN_TYPE_FLAG_BIT_NONE)) {
        tokens.emplace_back(current);
    }

    size_t componentCount = 0u;

    enum {
        EXPECTED_TYPE_DEFINITION,
        EXPECTED_TYPE_COMPONENT_MEMBER_DEFINITION_TYPE,
    } expected_type = EXPECTED_TYPE_DEFINITION;

    vector<variable_info> variableContext;

    for (auto ii = tokens.begin(); ii != tokens.end(); ) {
        if (expected_type == EXPECTED_TYPE_DEFINITION) {
            if (ii->type() == sxt::STX_TOKEN_TYPE_WORD) {
                if (ii->value() == "component") {
                    ii = predict_next(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});
                    const auto& name = ii->value();
                    definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_COMPONENT, .opcode = { name, to_string(componentCount) }});
                    ++componentCount;
                    ii = predict_next(ii, sxt::STX_TOKEN_TYPE_LCURLY, [](){exit(1);});
                    ++ii;

                    expected_type = EXPECTED_TYPE_COMPONENT_MEMBER_DEFINITION_TYPE;
                    continue;
                } else if (ii->value() == "struct") {
                    ii = predict_next(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});
                    const auto& name = ii->value();
                    definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_STRUCT, .opcode = { name }});
                    ii = predict_next(ii, sxt::STX_TOKEN_TYPE_LCURLY, [](){exit(1);});
                    ++ii;

                    expected_type = EXPECTED_TYPE_COMPONENT_MEMBER_DEFINITION_TYPE;
                    continue;
                } else {
                    exit(1);
                }
            } else if (ii->type() == sxt::STX_TOKEN_TYPE_TILDA) {
                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});
                const auto& returnTypename = ii->value();
                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});
                const auto& name = ii->value();


                definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_FUNCTION, .opcode = { returnTypename, name } });

                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_LPAREN, [](){exit(1);});

                // arguments parse here, TODO

                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_RPAREN, [](){exit(1);});

                // ii = predict_next(ii, sxt::STX_TOKEN_TYPE_LCURLY, [](){exit(1);});
                // ii = parse_function(ii, tokens.end(), variableContext, definitions);
                // definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_BODY_END, .opcode = { } });
                // ++ii;

                ++ii;
                if (ii->type() == sxt::STX_TOKEN_TYPE_LCURLY) {
                    ii = parse_function(ii, tokens.end(), variableContext, definitions);
                    definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_BODY_END, .opcode = { } });
                    ++ii;
                } else if (ii->type() == sxt::STX_TOKEN_TYPE_SEMICOLON) {
                    ++ii;
                } else {
                    ERROR_REPORT(ii->value() + " - unknown token type, maybe you mean `{`?\n");
                }

                expected_type = EXPECTED_TYPE_DEFINITION;
                continue;
            } else {
                ERROR_REPORT("unknown token type\n");
            }
        } else if (expected_type == EXPECTED_TYPE_COMPONENT_MEMBER_DEFINITION_TYPE) {
            if (ii->type() == sxt::STX_TOKEN_TYPE_WORD)  {
                const auto& memberTypename = ii->value();
                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_WORD, [](){exit(1);});
                const auto& memberName =  ii->value();
                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_SEMICOLON, [](){exit(1);});

                definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_MEMBER, .opcode = { memberTypename, memberName }});
                ++ii;

                expected_type = EXPECTED_TYPE_COMPONENT_MEMBER_DEFINITION_TYPE;

                continue;
            } else if (ii->type() == sxt::STX_TOKEN_TYPE_RCURLY) {
                ii = predict_next(ii, sxt::STX_TOKEN_TYPE_SEMICOLON, [](){exit(1);});
                ++ii;
                expected_type = EXPECTED_TYPE_DEFINITION;

                continue;
            } else {
                ERROR_REPORT("unknown token type, maybe you mean '}'?\n");
            }
        }
    }
    definitions.emplace_back(definition_info{.type = DEFINITION_TYPE_EOF, .opcode = {}}); // eof
}

string generate_c_functions(const vector<definition_info>& definitions) {
    string result;
    bool inFunction = false;
    for (size_t i = 0u; i < definitions.size(); ++i) {
        const definition_info& definition = definitions[i];

        if (definition.type == DEFINITION_TYPE_BODY_BEGIN) {
            result += "{\n";
        } else if (definition.type == DEFINITION_TYPE_BODY_END) {
            result += "}\n";
        } else if (inFunction) {
            if (definition.type == DEFINITION_TYPE_CREATE) {
                result += generate_c_create_ent_with_name(definition.opcode.at(0));
            } else if (definition.type == DEFINITION_TYPE_FOREACH_CYCLE) {
                result += generate_c_foreach(definition, definitions);
            } else if (definition.type == DEFINITION_TYPE_ADD_COMPONENTS) {
                result += generate_c_add_coponents(definition, definitions);
            } else if (definition.type == DEFINITION_TYPE_DESTROY_ENTITY) {
                result += generate_c_destroy_entity(definition.opcode.at(0));
            } else {
                inFunction = false;
            }
        }
        else {
            if (definition.type == DEFINITION_TYPE_FUNCTION) {
                result += definition.opcode.at(0) + " " + definition.opcode.at(1) + "() ";
                if ((i == (definitions.size() - 1)) || (definitions[i + 1].type != DEFINITION_TYPE_BODY_BEGIN)) {
                    result += ";\n";
                } else {
                    inFunction = true;
                }
            }
        }
    }
    return result;
}

int main() {
    string data =
    "struct point {\n"
    "\tfloat x;\n"
    "\tfloat y;\n"
    "};\n"
    "component position {\n"
    "\tpoint vector;\n"
    "\t}\n;"
    "component velocity {\n"
    "\tpoint vector;\n"
    "\t}\n;"
    "\n"
    "~int main();\n"
    "~int main() {\n"
    "\tent first;\n"
    "\tfirst.add<position, velocity>();\n"
    "\tfirst.destroy();\n"
    "\nforeach entity position { entity.destroy(); }\n"
    "}\n";

    sxt::tokenizer<string> tokenizer(data.begin(), data.end());
    vector<sxt::value_token<string>> tokens;
    for (sxt::value_token<string> current = tokenizer.next_new_token(sxt::STX_EXT_TOKEN_TYPE_FLAG_BIT_NONE); current.is_valid(); current = tokenizer.next_new_token(sxt::STX_EXT_TOKEN_TYPE_FLAG_BIT_NONE)) {
        tokens.emplace_back(current);
    }

    vector<definition_info> definitions;
    size_t componentCount = 0u;

    parse_definitions(data, definitions);
    // print "IR"
    // for (const auto& i : definitions) {
    //     cout << definition_type_to_string(i.type) << ' ';
    //     for (const auto& opcode : i.opcode) {
    //         cout << opcode << ' ';
    //     }
    //     cout << ";\n";
    // }
    cout << generate_c_start_code(definitions);
    cout << generate_c_structures(definitions);
    cout << generate_c_after_components_definition(definitions);
    cout << generate_c_functions(definitions);

    return 0;
}
