#pragma once

#include <string>
#include <vector>
#include <iostream>


namespace ArgumentParser {

template <typename T>
struct Argument {
    bool used = false;

    std::string short_name;
    std::string full_name;
    std::string description;

    T value;

    T default_value;
    bool has_default = false;

    T* storage = nullptr;

    Argument& Default(T default_val) {
        default_value = default_val;
        value = default_val;
        used = true;
        has_default = true;
        return *this;
    }

    std::string GetFullName() const {
        return full_name;
    }

    std::string GetShortName() const {
        return short_name;
    }

    std::string GetDescription() const {
        return description;
    }

    T GetValue() const {
        return value;
    }
};

template <typename T>
struct MultiArgument : public Argument<T>{
    bool is_multi = false;
    std::vector<T> arr_store;
    uint64_t min_count = 0;
    std::vector<T>* multi_storage = nullptr;
    bool is_positional = false;

    MultiArgument& StoreValue(T& store) {
        this->storage = &store;
        return *this;
    }

    MultiArgument& StoreValues(std::vector<T>& store) {
        multi_storage = &store;
        return *this;
    }

    MultiArgument& MultiValue(uint64_t count=0) {
        min_count = count;
        is_multi = true;
        return *this;
    }

    MultiArgument<int>& Positional();
};

struct IntArgument : public MultiArgument<int> {
    int GetValue() {
        return value;
    }

    int GetValue(int position) {
        return arr_store[position];
    }
};

struct StringArgument : public MultiArgument<std::string> {
    std::string GetValue() {
        return value;
    }

    std::string GetValue(int position) {
        return arr_store[position];
    }
};

struct Flag : public Argument<bool> {
    Flag& StoreValue(bool& store);
};

class ArgParser {
private:
    std::string parser_name_;
    std::string description_;
    std::vector<IntArgument*> int_arguments_;
    std::vector<StringArgument*> string_arguments_;
    std::vector<Flag*> flags_;
    StringArgument* help_argument_ = nullptr;

public:
    explicit ArgParser(const std::string& name);

    explicit ArgParser(const std::string& name, const std::string& description);

    bool Parse(const std::vector<std::string>& argv);

    bool Parser(const std::vector<std::string>& argv);

    uint64_t ParseInts(const std::vector<std::string>& data, const std::string& argument, uint64_t index);

    uint64_t ParseStrings(const std::vector<std::string>& data, const std::string& argument, uint64_t index);

    uint64_t ParseFlags(const std::vector<std::string>& data, const std::string& argument, uint64_t index);

    bool Parse(int size, char** arguments);

    void AddHelp(char short_name, const std::string& full_name,
                                  const std::string& arg_description);

    bool IsAllSpecified();

    bool Help() const;

    std::string HelpDescription() const;

    StringArgument& AddStringArgument(char short_name, const std::string& full_name,
                                             const std::string& description);

    StringArgument& AddStringArgument(char short_name, const std::string& full_name);

    StringArgument& AddStringArgument(const std::string& full_name, const std::string& description);

    StringArgument& AddStringArgument(const std::string& full_name);

    IntArgument& AddIntArgument(char short_name, const std::string& full_name,
                                  const std::string& description);

    IntArgument& AddIntArgument(const std::string& full_name);

    IntArgument& AddIntArgument(char short_name, const std::string& full_name);

    IntArgument& AddIntArgument(const std::string& full_name, const std::string& description);

    Flag& AddFlag(char short_name, const std::string& full_name,
                            const std::string& description);

    Flag& AddFlag(char short_name, const std::string& full_name);

    Flag& AddFlag(const std::string& full_name, const std::string& description);

    template <typename T>
    std::string GetStringValue(const T& argument_name) {
        auto name = std::string(argument_name);
        for (const auto& argument: string_arguments_) {
            if (argument->GetFullName() == name || argument->GetShortName() == name) {
                return argument->GetValue();
            }
        }

        throw std::invalid_argument("No such argument " + name + " in arguments list");
    }

    template <typename T>
    int GetIntValue(const T& argument_name) {
        auto name = std::string(argument_name);
        for (const auto& argument: int_arguments_) {
            if (argument->GetFullName() == name || argument->GetShortName() == name) {
                return argument->GetValue();
            }
        }

        throw std::invalid_argument("No such argument " + name + " in arguments list");
    }

    template <typename T>
    int GetIntValue(const T& argument_name, int position) {
        auto name = std::string(argument_name);
        for (const auto& argument: int_arguments_) {
            if (argument->GetFullName() == name || argument->GetShortName() == name) {
                return argument->GetValue(position);
            }
        }

        throw std::invalid_argument("No such argument " + name + " in arguments list");
    }

    template <typename T>
    bool GetFlag(const T& argument_name) {
        auto name = std::string(argument_name);
        for (const auto& argument: flags_) {
            if (argument->GetFullName() == name || argument->GetShortName() == name) {
                return argument->GetValue();
            }
        }

        throw std::invalid_argument("No such flag " + name + " in arguments list");
    }
};
} // namespace ArgumentParser