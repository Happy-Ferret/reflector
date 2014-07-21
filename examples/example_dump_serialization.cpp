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

#include <reflection/dump.hpp>

#include <cassert>
#include <string>

using namespace std;

class MyReader: public reflection::IReader, public reflection::ISeekBack {
public:
    MyReader(FILE* file) : file(file) {}

    virtual bool read(reflection::IErrorHandler* err, void* buffer, size_t count) override {
        if (fread(buffer, 1, count, file) != count) {
            if (feof(file))
                return err->error("UnexpectedEOF", "Unexpected end of file."), false;
            else
                return err->error("IOError", "Failed to read file."), false;
        }

        return true;
    }

    virtual void seekBack(long amount) override {
        fseek(file, -amount, SEEK_CUR);
    }

    FILE* file;
};

class MySchemaProvider : public reflection::ISchemaProvider {
public:
    virtual reflection::IReader* openClassSchemaOrNull(const char* className) override {
        string path = string("schemas/") + className + ".class_schema";
        FILE* file = fopen(path.c_str(), "rb");

        if (file == nullptr)
            return nullptr;

        // don't take this as a good example
        printf(" (using schema '%s')", path.c_str());

        return new MyReader(file);
    }

    virtual void closeClassSchema(reflection::IReader* reader) override {
        fclose(static_cast<MyReader*>(reader)->file);
        delete static_cast<MyReader*>(reader);
    }
};

bool str_ends_with(const char* str, const char* suffix) {
    if (str == nullptr || suffix == nullptr)
        return false;

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len)
        return false;

    return 0 == strncmp(str + str_len - suffix_len, suffix, suffix_len);
}

int main(int argc, char** argv) {
    assert(argc >= 2);

    const char* fileName = argv[1];
    FILE* file = fopen(fileName, "rb");
    assert(file != nullptr);

    MyReader rd(file);
    MySchemaProvider sp;

    if (str_ends_with(fileName, ".class"))
        reflection::dumpValue(reflection::TAG_CLASS, &rd, &rd, &sp);
    else if (str_ends_with(fileName, ".class_schema"))
        reflection::dumpValue(reflection::TAG_CLASS_SCHEMA, &rd, &rd, &sp);

    fclose(file);
}

#include <reflection/default_error_handler.cpp>
