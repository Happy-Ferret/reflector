/*
    Boost Software License - Version 1.0 - August 17, 2003

    Permission is hereby granted, free of charge, to any person or organization
    obtaining a copy of the software and accompanying documentation covered by
    this license (the "Software") to use, reproduce, display, distribute,
    execute, and transmit the Software, and to prepare derivative works of the
    Software, and to permit third-parties to whom the Software is furnished to
    do so, all subject to the following:

    The copyright notices in the Software and this entire statement, including
    the above license grant, this restriction and the following disclaimer,
    must be included in all copies of the Software, in whole or in part, and
    all derivative works of the Software, unless such copies or derivative
    works are solely in the form of machine-executable object code generated by
    a source language processor.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
    SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
    FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <reflection/api.hpp>
#include <reflection/magic.hpp>

#define ARG_REQUIRED(field_, spec_, description_)   REFL_MUST_CONFIG(field_, 0, spec_ "\0" description_)
#define ARG(field_, spec_, description_)            REFL_CONFIG(field_, 0, spec_ "\0" description_)

namespace argument_parsing {
struct Command_t {
    const char* commandName;
    const char* description;
    int (*execute)(const Command_t& cmd, int argc, char* argv[], const char* programName);
    void (*help)(const Command_t& cmd, const char* programName, bool full);
};

template <class Fields>
bool setDashArgument(Fields& fields, int argc, char* argv[], int& i, const char* programName, char argSpecified[]) {
    for (size_t j = 0; j < fields.count(); j++) {
        auto field = fields[j];

        const char* spec = field.params;

        if (spec[0] == '-' && spec[1] == argv[i][1]) {
            if (field.template isType<bool>()) {
                // -f
                if (!field.setFromString("1"))
                    return false;

                argSpecified[j] = 1;
                return true;
            }
            else if (argv[i][2] != 0) {
                // -fValue
                if (!field.setFromString(argv[i] + 2))
                    return false;

                argSpecified[j] = 1;
                return true;
            }
            else {
                // -f Value
                ++i;

                if (i >= argc) {
                    fprintf(stderr, "%s: error: expected '%s <%s>'\n", programName, spec, field.name);
                    return false;
                }

                if (!field.setFromString(argv[i]))
                    return false;

                argSpecified[j] = 1;
                return true;
            }
        }
    }

    fprintf(stderr, "%s: error: unrecognized argument '%s'\n", programName, argv[i]);
    return false;
}

template <class Fields>
bool setDashDashArgument(Fields& fields, int argc, char* argv[], int& i, const char* programName, char argSpecified[]) {
    for (size_t j = 0; j < fields.count(); j++) {
        auto field = fields[j];

        const char* spec = field.params;

        if (strcmp(spec, argv[i]) == 0) {
            if (field.template isType<bool>()) {
                // --option
                if (!field.setFromString("1"))
                    return false;

                argSpecified[j] = 1;
                return true;
            }
            else {
                // --option Value
                ++i;

                if (i >= argc) {
                    fprintf(stderr, "%s: error: expected '%s <%s>'\n", programName, spec, field.name);
                    return false;
                }

                if (!field.setFromString(argv[i]))
                    return false;

                argSpecified[j] = 1;
                return true;
            }
        }
    }

    fprintf(stderr, "%s: error: unrecognized argument '%s'\n", programName, argv[i]);
    return false;
}

template <class Fields>
bool setPlainArgument(Fields& fields, int argc, char* argv[], int& i, const char* programName, char argSpecified[]) {
    for (size_t j = 0; j < fields.count(); j++) {
        if (argSpecified[j])
            continue;

        auto field = fields[j];

        const char* spec = field.params;

        if (spec[0] == 0) {
            if (!field.setFromString(argv[i]))
                return false;

            argSpecified[j] = 1;
            return true;
        }
    }

    fprintf(stderr, "%s: error: unrecognized argument '%s'\n", programName, argv[i]);
    return false;
}

template <class Command>
int execute(const Command_t& cmd, int argc, char* argv[], const char* programName) {
    Command command;
    auto fields = reflection::reflectFields(command);

    // just an array of bools
    char* argSpecified = (char*) alloca(fields.count());
    memset(argSpecified, 0, fields.count());

    for (int i = 0; i < argc; i++) {
        const char* arg = argv[i];

        if (arg[0] == '-') {
            if (arg[1] != '-') {
                // dash argument

                if (!setDashArgument(fields, argc, argv, i, programName, argSpecified))
                    return -1;
            }
            else {
                // double-dash argument

                if (!setDashDashArgument(fields, argc, argv, i, programName, argSpecified))
                    return -1;
            }
        }
        else {
            if (!setPlainArgument(fields, argc, argv, i, programName, argSpecified))
                return -1;
        }
    }

    // check if all required arguments were specified
    for (size_t j = 0; j < fields.count(); j++) {
        const auto& field = fields[j];

        if ((field.systemFlags & reflection::FIELD_MANDATORY) && !argSpecified[j]) {
            fprintf(stderr, "%s: error: mandatory argument '%s' not specified\n\n", programName, field.name);
            cmd.help(cmd, programName, false);
            return -1;
        }
    }

    return command.execute();
}

template <typename Command>
void help(const Command_t& cmd, const char* programName, bool full) {
    auto fields = reflection::reflectFieldsStatic<Command>();

    if (cmd.commandName != nullptr)
        printf("usage: %s %s", programName, cmd.commandName);
    else
        printf("usage: %s", programName);

    for (size_t j = 0; j < fields.count(); j++) {
        const auto& field = fields[j];

        const char* spec = field.params;

        printf(" ");

        if (!(field.systemFlags & reflection::FIELD_MANDATORY))
            printf("[");

        if (spec[0]) {
            if (field.template isType<bool>())
                printf("%s", spec);
            else
                printf("%s <%s>", spec, field.name);
        }
        else
            printf("<%s>", field.name);

        if (!(field.systemFlags & reflection::FIELD_MANDATORY))
            printf("]");
    }

    printf("\n");

    if (!full)
        return;

    for (size_t j = 0; j < fields.count(); j++) {
        const auto& field = fields[j];

        const char* spec = field.params;
        const char* description = field.params + strlen(spec) + 1;

        printf("\t");

        if (spec[0]) {
            if (field.template isType<bool>())
                printf("%s", spec);
            else
                printf("%s <%s>", spec, field.name);
        }
        else
            printf("<%s>", field.name);

        printf("\t%s\n", description);
    }

    printf("\n");
}

template <typename Command>
int singleCommandDispatch(int argc, char* argv[], const char* programName, const char* description = nullptr) {
    static const Command_t cmd = {nullptr, description, execute<Command>, help<Command>};

    return cmd.execute(cmd, argc, argv, programName);
}

template <typename Command_t>
int multiCommandDispatch(int argc, char* argv[], const char* programName, const Command_t* commands) {
    if (argc < 1) {
        fprintf(stderr, "usage: '%s <command> <arguments...>'\n", programName);
        fprintf(stderr, "\tOR '%s help <command>'\n", programName);
        fprintf(stderr, "\tOR '%s help'\n", programName);
        fprintf(stderr, "\n");
        return -1;
    }

    if (strcmp(argv[0], "help") == 0) {
        if (argc < 2) {
            printf("available commands:\n");

            for (size_t i = 0; commands[i].commandName != nullptr; i++) {
                printf("\t%s\t%s\n", commands[i].commandName, commands[i].description);
            }

            printf("\ntype '%s help <command>' for more information about a specific command\n", programName);
            fprintf(stderr, "\n");
            return 0;
        }
        else {
            for (size_t i = 0; commands[i].commandName != nullptr; i++) {
                if (strcmp(commands[i].commandName, argv[1]) == 0) {
                    commands[i].help(commands[i], programName, true);
                    return 0;
                }
            }

            fprintf(stderr, "%s: error: unknown command '%s'\n", programName, argv[0]);
            return -1;
        }
    }

    for (size_t i = 0; commands[i].commandName != nullptr; i++) {
        if (strcmp(commands[i].commandName, argv[0]) == 0)
            return commands[i].execute(commands[i], argc - 1, argv + 1, programName);
    }

    fprintf(stderr, "%s: error: unknown command '%s'\n", programName, argv[0]);
    return -1;
}
}
