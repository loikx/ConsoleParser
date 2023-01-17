#include "ArgParser.h"

using namespace ArgumentParser;

template <>
MultiArgument<int>& MultiArgument<int>::Positional() {
    is_positional = true;
    return *this;
}

Flag& Flag::StoreValue(bool &store) {
    storage = &store;
    return *this;
}

ArgParser::ArgParser(const std::string& name) {
    parser_name_ = name;
}

ArgParser::ArgParser(const std::string& name, const std::string& description) {
    parser_name_ = name;
    description_ = description;
}

std::vector<std::string> GenerateGoodData(const std::vector<std::string>& argv) {
    std::vector<std::string> data;

    for (uint64_t i = 0; i < argv.size(); ++i) {
        if (argv[i][0] == '-' && std::isalpha(argv[i][1])) {
            if (std::find(argv[i].begin(), argv[i].end(), '=') == argv[i].end()) {
                uint64_t j = 1;
                while (argv[i][j] != '\0') {
                    data.emplace_back(std::string(1, argv[i][j]));
                    ++j;
                }
            } else {
                auto position = argv[i].find('=');
                data.push_back(argv[i].substr(1, position - 1));
                data.push_back(argv[i].substr(position + 1));
            }
        } else if (argv[i][0] == '-' && argv[i][1] == '-') {
            if (std::find(argv[i].begin(), argv[i].end(), '=') != argv[i].end()) {
                auto split_position = argv[i].find_first_of('=');
                data.push_back(argv[i].substr(2, split_position - 2));
                data.push_back(argv[i].substr(split_position + 1));
            } else {
                data.push_back(argv[i].substr(2));
            }
        } else {
            data.push_back(argv[i]);
        }
    }

    return std::move(data);
}

uint64_t ArgParser::ParseInts(const std::vector<std::string>& data, const std::string& argument, uint64_t index) {
    for (const auto& int_argument: int_arguments_) {
        if (int_argument->short_name == argument || int_argument->full_name == argument) {
            int_argument->used = true;
            if (int_argument->is_multi) {
                int_argument->arr_store.push_back(std::stoi(data[index + 1]));
                if (int_argument->multi_storage != nullptr) {
                    int_argument->multi_storage->push_back(std::stoi(data[index + 1]));
                }
            } else {
                int_argument->value = std::stoi(data[index + 1]);
                if (int_argument->storage != nullptr) {
                    *int_argument->storage = std::stoi(data[index + 1]);
                }
            }
            return index + 2;
        }
    }
    return index;
}

uint64_t ArgParser::ParseStrings(const std::vector<std::string>& data, const std::string& argument, uint64_t index) {
    for (const auto& string_argument: string_arguments_) {
        if (string_argument->short_name == argument || string_argument->full_name == argument) {
            string_argument->used = true;
            if (string_argument->is_multi) {
                string_argument->arr_store.push_back(data[index + 1]);
                if (string_argument->multi_storage != nullptr) {
                    string_argument->multi_storage->push_back(data[index + 1]);
                }
            } else {
                string_argument->value = data[index + 1];
                if (string_argument->storage != nullptr) {
                    *string_argument->storage = data[index + 1];
                }
            }
            return index + 2;
        }
    }
    return index;
}

uint64_t ArgParser::ParseFlags(const std::vector<std::string>& data, const std::string& argument, uint64_t index) {
    for (const auto& flag_argument: flags_) {
        if (flag_argument->short_name == argument || flag_argument->full_name == argument) {
            flag_argument->used = true;
            flag_argument->value = true;
            if (flag_argument->storage != nullptr) {
                *flag_argument->storage = true;
            }
            return index + 1;
        }
    }
    return index;
}


bool ArgParser::IsAllSpecified() {
    bool specified = true;

    for (const auto& item: int_arguments_) {
        specified = specified && item->used;
        if (item->is_multi) {
            specified = specified && (item->arr_store.size() >= item->min_count);
        }
    }
    for (const auto& item: string_arguments_) {
        specified = specified && item->used;
    }

    if (help_argument_ != nullptr)
        return specified || help_argument_->used;

    return specified;
}

bool ArgParser::Parser(const std::vector<std::string>& argv) {
    std::vector<std::string> data = GenerateGoodData(argv);
    std::vector<int> positional_arguments;

    uint64_t index = 0;
    uint64_t last_index = 0;
    std::string argument;
    while (index < data.size()) {
        last_index = index;
        argument = data[index];
        index = ParseInts(data, argument, index);
        if (index >= data.size())
            break;
        argument = data[index];
        index = ParseStrings(data, argument, index);
        if (index >= data.size())
            break;
        argument = data[index];
        index = ParseFlags(data, argument, index);
        if (last_index == index) {
            if (help_argument_ != nullptr &&
                (argument == help_argument_->short_name || argument == help_argument_->full_name)) {
                help_argument_->used = true;
            } else {
                positional_arguments.push_back(std::stoi(data[index]));
            }
            ++index;
        }
    }

    for (const auto& int_argument: int_arguments_) {
        if (int_argument->is_positional) {
            for (const auto& value: positional_arguments) {
                int_argument->arr_store.push_back(value);
                if (int_argument->multi_storage != nullptr)
                    int_argument->multi_storage->push_back(value);
            }
            int_argument->used = true;
            break;
        }
    }

    return IsAllSpecified();
}

bool ArgParser::Parse(int size, char** arguments) {
    std::vector<std::string> tmp;
    for (int i = 1; i < size; ++i) {
        tmp.emplace_back(arguments[i]);
    }
    return Parser(tmp);
}

bool ArgParser::Parse(const std::vector<std::string>& argv) {
    std::vector<std::string> tmp;
    for (int i = 1; i < argv.size(); ++i) {
        tmp.emplace_back(argv[i]);
    }
    return Parser(tmp);
}

bool ArgParser::Help() const {
    return help_argument_ != nullptr && help_argument_->used;
}

std::string ArgParser::HelpDescription() const {
    std::string short_name;
    std::string full_name;
    std::string description;

    std::string tmp = parser_name_ + "\n";
    if (!description_.empty())
        tmp += description_ + "\n\n";

    for (const auto& argument: string_arguments_) {
        short_name = argument->GetShortName();
        full_name = argument->GetFullName();
        description = argument->GetDescription();

        if (!short_name.empty())
            tmp += "-" + short_name;
        if (!full_name.empty()) {
            if (!short_name.empty())
                tmp += ", --" + full_name + "=<string>";
            else
                tmp += "    --" + full_name + "=<string>";
        }

        if (!description.empty()) {
            tmp += " " + description;
        }

        if (argument->has_default) {
            tmp += ", [default = " + argument->GetValue() + "]";
        }
        if (argument->is_multi) {
            tmp += ", [repeated, min args = " + std::to_string(argument->min_count) + "]";
        }
        tmp += "\n";
    }

    for (const auto& argument: int_arguments_) {
        short_name = argument->GetShortName();
        full_name = argument->GetFullName();
        description = argument->GetDescription();

        if (!short_name.empty())
            tmp += "-" + short_name;
        if (!full_name.empty()) {
            if (!short_name.empty())
                tmp += ", --" + full_name + "=<int>";
            else
                tmp += "    --" + full_name + "=<int>";
        }

        if (!description.empty())
            tmp += " " + description;

        if (argument->has_default) {
            tmp += ", [default = " + std::to_string(argument->GetValue()) + "]";
        }
        if (argument->is_multi) {
            tmp += ", [repeated, min args = " + std::to_string(argument->min_count) + "]";
        }
        tmp += "\n";
    }

    for (const auto& argument: flags_) {
        short_name = argument->GetShortName();
        full_name = argument->GetFullName();
        description = argument->GetDescription();

        if (!short_name.empty())
            tmp += "-" + short_name;
        if (!full_name.empty()) {
            if (!short_name.empty())
                tmp += ", --" + full_name;
            else
                tmp += "    --" + full_name;
        }

        if (!description.empty())
            tmp += " " + description;

        if (argument->has_default) {
            if (argument->default_value)
                tmp += ", [default = true]";
            else
                tmp += ", [default = false]";
        }

        tmp += "\n";
    }

    if (help_argument_ != nullptr) {
        short_name = help_argument_->GetShortName();
        full_name = help_argument_->GetFullName();
        description = help_argument_->GetDescription();

        tmp += '\n';
        if (!short_name.empty())
            tmp += "-" + short_name;
        if (!full_name.empty()) {
            if (!short_name.empty())
                tmp += ", --" + full_name;
            else
                tmp += "    --" + full_name;
        }

        if (!description.empty())
            tmp += " " + description;

        tmp += '\n';
    }
    return tmp;
}

void ArgParser::AddHelp(char short_name, const std::string& full_name,
                                         const std::string& arg_description) {
    auto* help_arg = new StringArgument();

    help_arg->short_name = short_name;
    help_arg->full_name = full_name;
    help_arg->description = arg_description;

    help_argument_ = help_arg;
}

StringArgument& ArgParser::AddStringArgument(char short_name, const std::string& full_name) {
    auto* new_string_argument = new StringArgument();

    new_string_argument->short_name = short_name;
    new_string_argument->full_name = full_name;
    string_arguments_.push_back(new_string_argument);

    return *new_string_argument;
}

StringArgument& ArgParser::AddStringArgument(char short_name, const std::string& full_name,
                                                    const std::string& description) {
    auto* new_string_argument = new StringArgument();

    new_string_argument->short_name = short_name;
    new_string_argument->full_name = full_name;
    new_string_argument->description = description;
    string_arguments_.push_back(new_string_argument);

    return *new_string_argument;
}

StringArgument& ArgParser::AddStringArgument(const std::string& full_name, const std::string& description) {
    auto* new_string_argument = new StringArgument();

    new_string_argument->full_name = full_name;
    new_string_argument->description = description;
    string_arguments_.push_back(new_string_argument);

    return *new_string_argument;
}

StringArgument& ArgParser::AddStringArgument(const std::string& full_name) {
    auto* new_string_argument = new StringArgument();

    new_string_argument->full_name = full_name;
    string_arguments_.push_back(new_string_argument);

    return *new_string_argument;
}

IntArgument& ArgParser::AddIntArgument(char short_name, const std::string& full_name,
                                         const std::string& description) {
    auto* new_int_argument = new IntArgument();

    new_int_argument->short_name = short_name;
    new_int_argument->full_name = full_name;
    new_int_argument->description = description;
    int_arguments_.push_back(new_int_argument);

    return *new_int_argument;
}

IntArgument& ArgParser::AddIntArgument(char short_name, const std::string& full_name) {
   auto* new_int_argument = new IntArgument();

   new_int_argument->short_name = short_name;
   new_int_argument->full_name = full_name;
    int_arguments_.push_back(new_int_argument);

    return *new_int_argument;
}

IntArgument& ArgParser::AddIntArgument(const std::string &full_name) {
    auto* new_int_argument = new IntArgument();

    new_int_argument->full_name = full_name;
    int_arguments_.push_back(new_int_argument);

    return *new_int_argument;
}

IntArgument& ArgParser::AddIntArgument(const std::string& full_name,
                                         const std::string& description) {
    static auto* new_int_argument = new IntArgument();

    new_int_argument->full_name = full_name;
    new_int_argument->description = description;
    int_arguments_.push_back(new_int_argument);

    return *new_int_argument;
}

Flag& ArgParser::AddFlag(char short_name, const std::string& full_name) {
    auto* new_flag = new Flag();

    new_flag->short_name = short_name;
    new_flag->full_name = full_name;
    flags_.push_back(new_flag);

    return *new_flag;
}

Flag& ArgParser::AddFlag(char short_name, const std::string& full_name,
                                   const std::string& description) {
    auto* new_flag = new Flag();

    new_flag->short_name = short_name;
    new_flag->full_name = full_name;
    new_flag->description = description;
    flags_.push_back(new_flag);

    return *new_flag;
}

Flag& ArgParser::AddFlag(const std::string& full_name, const std::string& description) {
    auto* new_flag = new Flag();

    new_flag->full_name = full_name;
    new_flag->description = description;
    flags_.push_back(new_flag);

    return *new_flag;
}